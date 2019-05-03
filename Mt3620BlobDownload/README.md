# AzureSphereCookBook
## Azure Blob Storage blob download for Azure Sphere
3rd May 2019

Credit: M Hall

This project shows how to use the libcurl functionality to download a file from Azure Blob Storage. This solution may be used to pull data configuration files or firmware for remote MCU.

Key parts to this project:

1. app_manifest: define allowed connection point

```JSON
    "AllowedConnections": [ "azurespheremcuupdate.blob.core.windows.net" ],
```
2. Generate blob SAS token and URL. This can be done from the Storage Explorer in the Azure Portal. Set fileName to the generated URL.
```c
	const char *fileName = "https://<storageaccount>.blob.core.windows.net/<blob and SAS access key";
```
3. Define required curl writefunction. This example dumps the file contents in a formated hex dump.
```c
size_t write_data(void *ptr, size_t size, size_t nmemb, struct url_data *data)
{
	// ptr == received data
	// nmemb == number of bytes

	totalSizeReceived += nmemb;
	DumpBuffer(ptr, nmemb);

	return size * nmemb;
}
```
4. Define the curl operation
```c
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
```
