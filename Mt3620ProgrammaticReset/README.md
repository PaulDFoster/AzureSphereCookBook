# AzureSphereCookBook
## Programmatic Reset of Azure Sphere
3rd May 2019

Azure Sphere connects to Azure Sphere Security Service (AS3) on a 24hour cycle. On power up teh Azure Sphere device completes authentication with AS3 and a certificate is provided to the Azure Sphere device with a 24hour lifetime. When this certificate expries the Azure Sphere device repeats the attestation to obtain a new certificate. In doing so, a check for the latest software is made.

The frequency of software updates can be increased by power cycling(reseting) the Azure Sphere, so that it repeats the authentication process with AS3.

This code project shows how a programmatic reset of the Azure Sphere RDB can be performed.

By connecting the SYS_RST_B pin (Header 1, Pin 1) to a GPIO, GPIO059 is used (Header 1, Pin 3), a reset can be caused by pulling GPIO059 low.

This code project befores the pulling of GPIO059 low when button A is pressed.

Key parts to this project:

1. app_manifest: add 59 to list of GPIO used

```json
    "Gpio": [ 8, 9, 10, 12, 59 ],
```
2. Initialise the GPIO for output with high value

```c
	// Setup GPIO059 as output with HIGH value
	// RESET operates when pulled LOW
	Log_Debug("Opening MT3620_GPIO59 as output.\n");
	gpioResetFd = GPIO_OpenAsOutput(MT3620_GPIO59, GPIO_OutputMode_OpenDrain, GPIO_Value_High);
	if (gpioResetFd < 0) {
		Log_Debug("ERROR: Could not open RESET GPIO: %s (%d).\n", strerror(errno), errno);
		return -1;
	}
```
3. Detect button press and cause a reset
```c
    // If the button has just been pressed, cause a reset
    // The button has GPIO_Value_Low when pressed and GPIO_Value_High when released
    if (newButtonState != buttonState) {
        if (newButtonState == GPIO_Value_Low) {

			int result = GPIO_SetValue(gpioResetFd, GPIO_Value_Low);

			// Reset board will occur
			// Execution never gets here
			if (result != 0) {
				Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
				terminationRequired = true;
				return;
			}

        }
        buttonState = newButtonState;
    }
```


