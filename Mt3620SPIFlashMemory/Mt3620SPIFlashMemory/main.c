#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include "epoll_timerfd_utilities.h"

#include <applibs/log.h>
#include <applibs/spi.h>

#include "mt3620_rdb.h"

// This C application for the MT3620 Reference Development Board (Azure Sphere)
// uses a 1MB SPI Flash Memory chip and outputs results to Visual Studio's Device Output window
//
// It is a simple port of an arduino project by PTorelli published on instructables
// https://www.instructables.com/id/How-to-Design-with-Discrete-SPI-Flash-Memory/
//
// It uses the API for the following Azure Sphere application libraries:
// - log (messages shown in Visual Studio's Device Output window during debugging)

static volatile sig_atomic_t terminationRequired = false;

static int spiFd = -1;

// WinBond flash commands
const uint8_t WB_WRITE_ENABLE = 0x06 ;
const uint8_t WB_WRITE_DISABLE = 0x04;
const uint8_t WB_CHIP_ERASE = 0xc7 ;
const uint8_t WB_READ_STATUS_REG_1 = 0x05;
const uint8_t WB_READ_DATA = 0x03 ;
const uint8_t WB_PAGE_PROGRAM = 0x02 ;
const uint8_t WB_JEDEC_ID = 0x9f ;


/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    terminationRequired = true;
}


/*
 * print_page_bytes() is a simple helperf function that formats 256
 * bytes of data into an easier to read grid.
 */
void print_page_bytes(uint8_t *page_buffer) {
	for (int i = 0; i < 16; ++i) {
		for (int j = 0; j < 16; ++j) {
			Log_Debug("%02x: ", page_buffer[i * 16 + j]);
		}
		Log_Debug("\n");
	}
}


/// <summary>
///    Checks the number of transferred bytes for SPI functions and prints an error
///    message if the functions failed or if the number of bytes is different than
///    expected number of bytes to be transferred.
/// </summary>
/// <returns>true on success, or false on failure</returns>

static bool CheckTransferSize(const char *desc, size_t expectedBytes, ssize_t actualBytes)
{
	if (actualBytes < 0) {
		Log_Debug("ERROR: %s: errno=%d (%s)\n", desc, errno, strerror(errno));
		return false;
	}

	if (actualBytes != (ssize_t)expectedBytes) {
		Log_Debug("ERROR: %s: transferred %zd bytes; expected %zd\n", desc, actualBytes,
			expectedBytes);
		return false;
	}

	return true;
}


/*
================================================================================
Low-Level Device Routines
The functions below perform the lowest-level interactions with the flash device.
They match the timing diagrams of the datahsheet. They are called by wrapper
functions which provide a little more feedback to the user. I made them stand-
alone functions so they can be re-used. Each function corresponds to a flash
instruction opcode.
================================================================================
*/


/*
 * not_busy() polls the status bit of the device until it
 * completes the current operation. Most operations finish
 * in a few hundred microseconds or less, but chip erase
 * may take 500+ms. Finish all operations with this poll.
 *
 * See section 9.2.8 of the datasheet
 */
void not_busy(void) {

	uint8_t buf = 0;
	size_t szbuf = sizeof(buf);
	size_t sz = sizeof(WB_READ_STATUS_REG_1);

	SPIMaster_WriteThenRead(spiFd, &WB_READ_STATUS_REG_1, sz, &buf, szbuf);

	while (buf & 1) {
		SPIMaster_WriteThenRead(spiFd, &WB_READ_STATUS_REG_1, sz, &buf, szbuf);
	}

}


/*
 * See the timing diagram in section 9.2.35 of the
 * data sheet, "Read JEDEC ID (9Fh)".
 */
void _get_jedec_id(uint8_t *b1, uint8_t *b2, uint8_t *b3) {

	uint8_t buf[3] = { 0,0,0 };
	size_t szbuf = sizeof(buf);

	SPIMaster_WriteThenRead(spiFd, &WB_JEDEC_ID, 1, buf, szbuf);

	*b1 = buf[0]; // manufacturer id
	*b2 = buf[1]; // memory type
	*b3 = buf[2]; // capacity

	not_busy();
}

/*
 * See the timing diagram in section 9.2.26 of the
 * data sheet, "Chip Erase (C7h / 06h)". (Note:
 * either opcode works.)
 */
int _chip_erase(void) {
	static const size_t transferCount = 1;
	SPIMaster_Transfer transfers[transferCount];

	int result = SPIMaster_InitTransfers(transfers, transferCount);

	if (result != 0) {
		return -1;
	}

	transfers[0].flags = SPI_TransferFlags_Write;
	transfers[0].writeData = &WB_WRITE_ENABLE;
	transfers[0].length = sizeof(WB_WRITE_ENABLE);

	ssize_t transferredBytes = SPIMaster_TransferSequential(spiFd, transfers, transferCount);

	if (!CheckTransferSize("Failed to write enable", sizeof(WB_WRITE_ENABLE), transferredBytes)) {
		return -1;
	}

	transfers[0].flags = SPI_TransferFlags_Write;
	transfers[0].writeData = &WB_CHIP_ERASE;
	transfers[0].length = sizeof(WB_CHIP_ERASE);

	transferredBytes = SPIMaster_TransferSequential(spiFd, transfers, transferCount);

	if (!CheckTransferSize("Failed to erase chip", sizeof(WB_CHIP_ERASE), transferredBytes)) {
		return -1;
	}

	not_busy();
	return 0;
}

/*
 * See the timing diagram in section 9.2.10 of the
 * data sheet, "Read Data (03h)".
 */
int _read_page(uint16_t page_number, uint8_t *page_buffer) {
	static const size_t transferCount = 5;
	SPIMaster_Transfer transfers[transferCount];

	int result = SPIMaster_InitTransfers(transfers, transferCount);

	if (result != 0) {
		return -1;
	}

	transfers[0].flags = SPI_TransferFlags_Write;
	transfers[0].writeData = &WB_READ_DATA;
	transfers[0].length = sizeof(WB_READ_DATA);

	// Construct the 24-bit address from the 16-bit page
	// number and 0x00, since we will read 256 bytes (one
	// page).
	const uint8_t shiftPage_number = (uint8_t)((page_number >> 8) & 0xFF);
	const uint8_t unshiftedPage_number = (uint8_t)((page_number >> 0) & 0xFF);
	const uint8_t noop = 0;

	transfers[1].flags = SPI_TransferFlags_Write;
	transfers[1].writeData = &shiftPage_number;
	transfers[1].length = sizeof(shiftPage_number);

	transfers[2].flags = SPI_TransferFlags_Write;
	transfers[2].writeData = &unshiftedPage_number;
	transfers[2].length = sizeof(unshiftedPage_number);

	// 3rd byte of 24 bit address
	transfers[3].flags = SPI_TransferFlags_Write;
	transfers[3].writeData = &noop;
	transfers[3].length = sizeof(noop);

	transfers[4].flags = SPI_TransferFlags_Read;
	transfers[4].readData = page_buffer;
	transfers[4].length = sizeof(char[256]);

	ssize_t transferredBytes = SPIMaster_TransferSequential(spiFd, transfers, transferCount);

	if (!CheckTransferSize("Failed to read page", sizeof(WB_READ_DATA) + 
													sizeof(shiftPage_number) + 
													sizeof(unshiftedPage_number) +
													sizeof(noop) +
													sizeof(char[256]), transferredBytes)) {
		return -1;
	}

	not_busy();
	return 0;
}

/*
 * See the timing diagram in section 9.2.21 of the
 * data sheet, "Page Program (02h)".
 */
int _write_page(uint16_t page_number, uint8_t *page_buffer) {
	static const size_t transferCount = 1;
	SPIMaster_Transfer transfers[transferCount];

	int result = SPIMaster_InitTransfers(transfers, transferCount);

	if (result != 0) {
		return -1;
	}

	transfers[0].flags = SPI_TransferFlags_Write;
	transfers[0].writeData = &WB_WRITE_ENABLE;
	transfers[0].length = sizeof(WB_WRITE_ENABLE);

	ssize_t transferredBytes = SPIMaster_TransferSequential(spiFd, transfers, transferCount);

	if (!CheckTransferSize("Failed to write enable", sizeof(WB_WRITE_ENABLE), transferredBytes)) {
		return -1;
	}

	static const size_t transferCountNxt = 5;
	SPIMaster_Transfer transfersNxt[transferCountNxt];
	result = SPIMaster_InitTransfers(transfersNxt, transferCountNxt);

	transfersNxt[0].flags = SPI_TransferFlags_Write;
	transfersNxt[0].writeData = &WB_PAGE_PROGRAM;
	transfersNxt[0].length = sizeof(WB_PAGE_PROGRAM);

	// Construct the 24-bit address from the 16-bit page
	// number and 0x00, since we will read 256 bytes (one
	// page).
	const uint8_t shiftPage_number = (uint8_t)(page_number >> 8) & 0xFF;
	const uint8_t unshiftedPage_number = (uint8_t)(page_number >> 0) & 0xFF;
	const uint8_t noop = 0;

	transfersNxt[1].flags = SPI_TransferFlags_Write;
	transfersNxt[1].writeData = &shiftPage_number;
	transfersNxt[1].length = sizeof(shiftPage_number);

	transfersNxt[2].flags = SPI_TransferFlags_Write;
	transfersNxt[2].writeData = &unshiftedPage_number;
	transfersNxt[2].length = sizeof(unshiftedPage_number);

	// 3rd byte of 24 bit address
	transfersNxt[3].flags = SPI_TransferFlags_Write;
	transfersNxt[3].writeData = &noop;
	transfersNxt[3].length = sizeof(noop);

	transfersNxt[4].flags = SPI_TransferFlags_Write;
	transfersNxt[4].writeData = page_buffer;
	transfersNxt[4].length = sizeof(char[256]);

	transferredBytes = SPIMaster_TransferSequential(spiFd, transfersNxt, transferCountNxt);

	if (!CheckTransferSize("Failed to write page", sizeof(WB_PAGE_PROGRAM) +
													sizeof(shiftPage_number) + 
													sizeof(unshiftedPage_number) +
													sizeof(noop) + 
													sizeof(char[256]), transferredBytes)) {
		return -1;
	}

	not_busy();
	return 0;
}



/*
================================================================================
User Interface Routines
The functions below map to user commands. They wrap the low-level calls with
print/debug statements for readability.
================================================================================
*/

/*
 * The JEDEC ID is fairly generic, I use this function to verify the setup
 * is working properly.
 */
void get_jedec_id(void) {
	Log_Debug("command: get_jedec_id\n");
	uint8_t b1, b2, b3;
	_get_jedec_id(&b1, &b2, &b3);
	Log_Debug("Manufacturer ID: %02xh\nMemory Type: %02xh\nCapacity: %02xh\n", b1, b2, b3);
	Log_Debug("Ready\n");
}

void chip_erase(void) {
	Log_Debug("command: chip_erase\n");
	_chip_erase();
	Log_Debug("Ready\n");
}

void read_page(uint16_t page_number) {
	Log_Debug( "command: read_page(%04xh)\n", page_number);
	uint8_t page_buffer[256];

	for (int i = 0; i < 256; i++)
	{
		page_buffer[i] = 0;
	}

	_read_page(page_number, page_buffer);
	print_page_bytes(page_buffer);
	Log_Debug("Ready\n");
}

void read_all_pages(void) {
	Log_Debug("command: read_all_pages\n");
	uint8_t page_buffer[256];
	for (uint16_t i = 0; i < 4096; ++i) {
		_read_page(i, page_buffer);
		print_page_bytes(page_buffer);
	}
	Log_Debug("Ready\n");
}

void write_byte(uint16_t page, uint8_t offset, uint8_t databyte) {
	Log_Debug( "command: write_byte(%04xh, %04xh, %02xh)\n", page, offset, databyte);
	uint8_t page_data[256];
	_read_page(page, page_data);
	page_data[offset] = databyte;
	_write_page(page, page_data);
	Log_Debug("Ready\n");
}


static int Setup(void)
{

	SPIMaster_Config config;
	int ret = SPIMaster_InitConfig(&config);
	if (ret != 0) {
		Log_Debug("ERROR: SPIMaster_InitConfig = %d errno = %s (%d)\n", ret, strerror(errno),
			errno);
		return -1;
	}
	config.csPolarity = SPI_ChipSelectPolarity_ActiveLow;
	spiFd = SPIMaster_Open(MT3620_SPI_ISU0, MT3620_SPI_CHIP_SELECT_A, &config);
	if (spiFd < 0) {
		Log_Debug("ERROR: SPIMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	int result = SPIMaster_SetBusSpeed(spiFd, 400000);
	if (result != 0) {
		Log_Debug("ERROR: SPIMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	result = SPIMaster_SetMode(spiFd, SPI_Mode_0);
	if (result != 0) {
		Log_Debug("ERROR: SPIMaster_SetMode: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	return 0;
}



/// <summary>
///     Main entry point for this sample.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("Application starting.\n");

    // Register a SIGTERM handler for termination requests
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

	// Set up SPI
	Setup();

	// Get flash memory chip id
	get_jedec_id();
	chip_erase();
	read_page(0);

	for (int i = 1; i < 10; i++)
	{
		write_byte(0, i, 8);
	}

	read_page(0);

    // Main loop
    const struct timespec sleepTime = {1, 0};
    while (!terminationRequired) {
        //Log_Debug("Done\n");
        nanosleep(&sleepTime, NULL);
    }

    Log_Debug("Application exiting.\n");
    return 0;
}
