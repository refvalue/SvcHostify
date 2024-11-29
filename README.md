# SvcHostify: Hosting Your Own Codebase as A SvcHost DLL Service Easily



## Motivation

`svchost.exe` is a critical system process for hosting DLLs as Windows Services, but some of its internal workings are undocumented. Creating custom `svchost` DLL services, especially when using languages like Java or C#, can be cumbersome. This is mainly due to the need for exporting C-style functions to interact with `svchost.exe`, which complicates the development process. To address this challenge, I created the "svchostify" project, which simplifies the creation of `svchost` DLL services, making it easier for developers to work with `svchost.exe` and supporting a wider range of programming languages.



## Prerequisites

The latest [Visual C++ Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist) is required for everything to function properly, only for x64 Windows.



## Downloading the Hosting DLL

This project provides a C++ dynamic library that enables hosting a DLL service, which can be registered to be loaded by `svchost.exe`. The latest compiled release can be found on the [Release](../../releases/) page. The default name for the prebuilt DLL is typically `svchostify.dll`. You can download and extract it to any location on your disk that you prefer.



## Command-Line Overview

The `svchostify.dll` is a self-executable library, meaning it can be directly loaded by `rundll32.exe`—the Windows tool used to run DLLs—by locating the C-style function entry specified by the user.

```powershell
rundll32 svchostify.dll invoke <OPTIONS>
```

Available options and arguments are listed below:

| Parameter             | Description                                                  |
| --------------------- | ------------------------------------------------------------ |
| `--install`, `-i`     | Installs the service.                                        |
| `--uninstall`, `-u`   | Uninstalls the service.                                      |
| `--config-file`, `-c` | Specifies a configuration file that the installation or uninstallation is based on. |
| `--help`, `-h`        | Displays details of the command line arguments.              |

To install or uninstall a service, use:

```powershell
rundll32 svchostify.dll invoke -i -c <CONFIG_FILE>
rundll32 svchostify.dll invoke -u -c <CONFIG_FILE>
```

**NOTE: The configuration payload, along with the service metadata, will be written to the Windows Registry simultaneously, eliminating the need for a config file after installation.**



## JSON Configuration

The hosting service reads a JSON file for its internal configuration and the key-value pairs and objects are defined as follows:

| Field Name         | Type              | Description                                                  | Possible Values                                 | Default          | Required |
| ------------------ | ----------------- | ------------------------------------------------------------ | ----------------------------------------------- | ---------------- | -------- |
| `workType`         | `string`          | The type of the service.                                     | `com`, `pure_c`, `jvm`, `executable`            |                  | Yes      |
| `name`             | `string`          | The name of the service.                                     | `Any`                                           |                  | Yes      |
| `displayName`      | `string`          | The display name of the service.                             | `Any`                                           |                  | Yes      |
| `context`          | `string`          | Arbitrary content according to the work type: a coclass `{GUID}` for `com`, a DLL path for `pure_c`, a `CLASSPATH` for `jvm`, an EXE path for `executable`. | `Any`                                           |                  | Yes      |
| `accountType`      | `string`          | The account type under which the service runs.               | `localSystem`, `networkService`, `localService` |                  | Yes      |
| `postQuitMessage`  | `boolean`         | Indicates whether to post a quit message before the service exits when the type is `executable`. | `true`, `false`                                 | `false`          | No       |
| `description`      | `string`          | A description of the service.                                | `Any`                                           | `null`           | No       |
| `jdkDirectory`     | `string`          | The JDK Directory.                                           | Any valid directory path                        | `null`           | No       |
| `workingDirectory` | `string`          | The initial working directory.                               | Any valid directory path                        | The DLL location | No       |
| `arguments`        | `array of string` | The startup arguments for the service.                       | `List of string`                                | `null`           | No       |
| `dllDirectories`   | array of string   | Additional directories for loading DLLs.                     | List of directories                             | The DLL location | No       |
| `logger`           | `object`          | Logger configuration object.                                 | See below                                       | See below        | No       |

#### Logger Configuration Object

| Field Name | Type     | Description                                                  | Possible Values                | Default            | Required |
| ---------- | -------- | ------------------------------------------------------------ | ------------------------------ | ------------------ | -------- |
| `basePath` | `string` | The base path of the logging file.                           | Any valid directory path       | log/svchostify.log | Yes      |
| `maxSize`  | `string` | The maximum size of one single log file. The pattern is `\\d+\\s*(KiB|MiB|GiB|TiB)?`. | Any valid values like `10 MiB` | 50 MiB             | No       |
| `maxFiles` | `number` | The maximum count of log files.                              | Any positive integer           | 5                  | No       |

**Note: The complete JSON schema can be found [here](svchostify.schema.json).**



## JSON Samples

### Hosting a JAR package

```json
{
  "workerType": "jvm",
  "name": "ABC_SomeTestSvc_Java",
  "displayName": "ABC Test Java Service",
  "context": "E:/Projects/SvcHostify/samples/svchost.jar",
  "accountType": "networkService",
  "description": "A Test Java Service for SvcHost.",
  "jdkDirectory": "D:/jdk-21",
  "workingDirectory": "D:/",
  "arguments": [
    "Hello",
    "World"
  ],
  "logger": {
    "basePath": "logs/logger_java.log",
    "maxSize": "10 MiB",
    "maxFiles": 10
  }
}
```

The sample project is [located here](samples/java/).



### Hosting a C# DLL implemented as an in-process COM server

```json
{
  "workerType": "com",
  "name": "ABC_MyTestSvc_CSharp",
  "displayName": "ABC Test CSharp Service",
  "context": "{47D7093F-69E2-D17D-422D-49BE836EF3A5}",
  "accountType": "localSystem",
  "description": "Test CSharp (Hosted as a COM server) Service for SvcHost.",
  "workingDirectory": "D:/",
  "arguments": [
    "Hello",
    "World"
  ],
  "logger": {
    "basePath": "logs/logger_csharp.log",
    "maxSize": "10 MiB",
    "maxFiles": 10
  }
}
```

The sample project can be [checked here](samples/csharp/).



### Hosting a C++ DLL by calling exported pure C functions

```json
{
  "workerType": "pureC",
  "name": "ABC_MyTestSvc_CPP",
  "displayName": "ABC Test C++ Service",
  "context": "refvalue-test-service.dll",
  "accountType": "localSystem",
  "description": "Test C++ Service for SvcHost.",
  "workingDirectory": "D:/",
  "arguments": [
    "Hello",
    "World"
  ],
  "logger": {
    "basePath": "logs/logger_cpp.log",
    "maxSize": "10 MiB",
    "maxFiles": 10
  }
}
```

The sample project is provided for [review here](samples/cpp/).



## Calling Conventions

| Programming Language | Calling Method                                               |
| -------------------- | ------------------------------------------------------------ |
| Java                 | Implements `org.refvalue.SvcHostify` class with `static` methods `void run(String[] args)` and `void onStop()` |
| C#                   | Implements the COM interface `ISvcHostify` and its methods `void Run(string[] args)` and `void OnStop()` |
| C/C++                | Exports `extern "C"` functions `void refvalue_svchostify_run(std::size_t argc, char* argv[])` and `void refvalue_svchostify_on_stop()` |

Some quick samples are provided in the `samples` directory. Feel free to [take a look](samples/)!



## Logging Redirection

This system automatically redirects all output sent to `stdout` and `stderr` streams to a log file. It captures logs from various sources, including:

- **Java**: `System.out.println`
- **C#**: `Console.WriteLine`
- **C++**: `std::cout`, `spdlog`, `printf`and other standard output methods

The log file is continuously updated, with a new entry added every time the output exceeds 4 KB, ensuring that log data is refreshed at regular intervals. This redirection simplifies logging by consolidating output from multiple languages and libraries into a single, centralized log file for easier monitoring and troubleshooting.



## LICENSE

This project is licensed under the MIT License.

You are free to use, modify, and distribute the software, including for commercial purposes, under the terms of the MIT License.
See the [LICENSE](LICENSE) file for more details.
