#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>

#include "mt3620_rdb.h"

#include <curl/curl.h>
#include <curl/easy.h>

bool DownloadMLModel(void);
void DumpBuffer(uint8_t *buffer, uint16_t length);
void DisplayLineOffset(uint16_t offset);

// This C application for the MT3620 Reference Development Board (Azure Sphere)
// outputs a string every second to Visual Studio's Device Output window
//
// It uses the API for the following Azure Sphere application libraries:
// - log (messages shown in Visual Studio's Device Output window during debugging)

static volatile sig_atomic_t terminationRequired = false;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
	// Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
	terminationRequired = true;
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

	DownloadMLModel();

	// Main loop
	const struct timespec sleepTime = { 1, 0 };
	while (!terminationRequired) {
		Log_Debug("Blob processed\n");
		nanosleep(&sleepTime, NULL);
	}

	Log_Debug("Application exiting.\n");
	return 0;
}

uint8_t downloadBuffer[512] = { 0 };

struct url_data {
	size_t size;
	char* data;
};

unsigned long totalSizeReceived = 0;

size_t write_data(void *ptr, size_t size, size_t nmemb, struct url_data *data);

bool DownloadMLModel(void)
{

	const char *fileName = "https://<storageaccount>.blob.core.windows.net/<blob and SAS access key";

	CURL *curl;
	CURLcode res;

	struct url_data data;
	data.size = 0;
	data.data = malloc(4096); /* reasonable size initial buffer */

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();


	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, fileName);
		/* use a GET to fetch this */
		curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
		/* Perform the request */
		res = curl_easy_perform(curl);

		Log_Debug("Total Size Received: %lu bytes\n", totalSizeReceived);
	}

	if (res > 0) {
		return true;
	}
	else {
		return false;
	}

}

size_t write_data(void *ptr, size_t size, size_t nmemb, struct url_data *data)
{
	// ptr == received data
	// nmemb == number of bytes

	totalSizeReceived += nmemb;
	DumpBuffer(ptr, nmemb);

	return size * nmemb;
}

void DumpBuffer(uint8_t *buffer, uint16_t length)
{
	char ascBuff[17] = { 0 };
	int LinePos = 0;

	uint16_t lineOffset = 0;

	DisplayLineOffset(lineOffset);
	memset(ascBuff, 0x20, 16);
	for (int x = 0; x < length; x++)
	{
		Log_Debug("%02x ", buffer[x]);
		if (buffer[x] >= 0x20 && buffer[x] <= 0x7f)
			ascBuff[LinePos] = buffer[x];
		else
			ascBuff[LinePos] = '.';

		LinePos++;
		if (LinePos == 0x10)
		{
			Log_Debug("%s", ascBuff);
			Log_Debug("\n");
			lineOffset += 16;
			if (x + 1 != length)
			{
				DisplayLineOffset(lineOffset);
			}
			LinePos = 0;
			memset(ascBuff, 0x20, 16);
		}
	}

	if (LinePos != 0)
		Log_Debug("%s", ascBuff);

	Log_Debug("\n");
}

void DisplayLineOffset(uint16_t offset)
{
	Log_Debug("%04x: ", offset);
}

