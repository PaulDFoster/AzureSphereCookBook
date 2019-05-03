# AzureSphereCookBook
## Simple SPI
3rd May 2019

This project shows a minmial configuration for connecting an AZure Sphere RDB to another slave board via SPI. The project was created to demonstrate communication to an ESP8266.

The project performs no useful function other than to show the set up and execution of SPI writes and reads which can be easily validated using a Logic Analyser connected across the bus.

1. Set the project properties to target BETA+1 api
2. app_manifest: set the ISU that will be used for SPIMaster
```JSON
    "SpiMaster": ["ISU0"],
```
NOTE: On the developers RDB it was discovered that ISU1 delivered a clock signal on MISO. If you observe this behaviour via your Logic Analyser please use a different ISU.

3. Set up the SPIMaster configuration
```c
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
```
4. The simple SPI process. This uses the Azure Sphere SPIMaster_WriteThenRead function. POSIX write and read functions are also available.
```c
static void SPIProcess(void)
{
	char buf[4] = { 0x08,0x02,0x03,0x04 };
	char rbuf[10] = { 0,0,0,0,0,0,0,0,0,0 };
	size_t count = 1;
	size_t rcount = 1;

	SPIMaster_WriteThenRead(spiFd, buf, count, rbuf, rcount);

	for (int i = 0; i < 10; i++) {
		Log_Debug("%i = %i\n", i, rbuf[i]);
	}
}
```
