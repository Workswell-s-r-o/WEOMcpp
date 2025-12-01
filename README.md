# Thermal Core SDK Example

This project demonstrates the basic usage of the `thermal-core` SDK to connect to a thermal camera device, read properties, and handle a video stream in a multi-threaded environment.

## Features

*   **Device Connection:** Establishes a connection to a WTC640 thermal camera via a serial port.
*   **Multi-threaded Property Access:** Shows how to safely access device properties from multiple threads using the properties transaction system.
*   **Exclusive vs. Normal Transactions:** Demonstrates the difference between a blocking exclusive transaction and a non-blocking properties transaction.
*   **Video Stream Handling:** A dedicated thread is used to manage the video stream and read image data.
*   **Logging:** Utilizes the built-in logging framework to provide information about the application's state.

## Prerequisites

*   **CMake:** For generating the build system.
*   **A C++20 compliant compiler:** (e.g., GCC, Clang, MSVC).
*   **Conan (Optional):** The project can use Conan for C++ package management. If you do not wish to use Conan, you will need to provide the dependencies manually.

## Building the Example

This project uses CMake.

### With Conan (Recommended)

If you are using Conan, the dependencies will be fetched automatically when you run CMake.

1.  Create a build directory:
    ```sh
    mkdir build
    cd build
    ```

2.  Run CMake to configure the project:
    ```sh
    cmake ..\thermal-core -DW_BUILD_EXAMPLE:BOOL=ON -DW_CORE_BUILD_WTC640:BOOL=ON -DCMAKE_BUILD_TYPE=Release
    ```
    
    Optionally if you want to use conan as a package manager make sure to add 
    ```sh
    -DW_BUILD_DEPENDENCIES_WITH_CONAN:BOOL=ON
    ```
If you need debug symbols in your release build (for example, if some Conan packages do not have a debug recipe), you can add the following flag:
    ```sh    -DW_DEBUG_SYMBOLS_IN_RELEASE:BOOL=ON
    ```

3.  Build the project:
    ```sh
    cmake --build . --config Release
    ```

### Without Conan

If you are not using Conan, you will need to ensure that all required libraries are found by CMake. You may need to set `CMAKE_PREFIX_PATH` or other variables to point to your library installations. The build steps are otherwise the same.

## Running the Example

The compiled executable will be located in the `build` directory (or a subdirectory, depending on your generator).

**Note:** You need to provide 2 commandline arguments to the example, the first being the SN of the USB Plugin, for example `20013-073-2412` and the second being the systemlocation of the serial port, on Windows `\\.\COM37` and on linux `/dev/tty0`

When you run the application, you will see log output in your console showing the interaction between the main thread and the video thread. The main thread will periodically acquire an exclusive lock to read the device's article number, during which the video thread will be unable to acquire its own lock to read the shutter temperature.

## Purpose of the Example

The primary purpose of this example is to demonstrate a realistic multi-threaded usage of the `thermal-core` SDK. It showcases how to manage a video stream in one thread while performing blocking, critical device operations in another, using the properties transaction system to prevent conflicts.

Key demonstrations include:
-   **Exclusive vs. Normal Transactions:** It highlights the difference between a blocking exclusive transaction (`createConnectionExclusiveTransactionWtc640`) and a non-blocking attempt to gain access (`tryCreatePropertiesTransaction`).
-   **Thread-Safe Property Access:** It shows the intended way to access and modify device properties from multiple threads safely. The main thread takes a "strong" lock to change a palette, while the video thread uses a "weak" lock to read sensor data, yielding when the main thread has control.
-   **Asynchronous Callbacks:** It shows a handler being connected to the `transactionFinished` signal, which is invoked asynchronously when a property changes.

## Code Flow

The application's logic is contained within the `Example` class.

1.  **Initialization (`main` and `Example` constructor):**
    -   The `main` function instantiates the `Example` object.
    -   The `Example` constructor initializes the logging framework and the `thermal-core` properties system in `ASYNC_QUEUED` mode.
    -   It connects a C++ lambda function to the `transactionFinished` signal to react to property changes. In this case, it logs a message whenever the color palette index is changed.

2.  **Connection and Stream Setup (`Example::run`):**
    -   The `run` method first defines the connection parameters for the camera's serial port.
    -   It creates a `connectionStateTransaction` to establish a connection with the device.
    -   Upon connecting, it uses an **exclusive transaction** to safely get to the `IStream` video stream object and starts the stream. Using an exclusive lock here ensures that no other thread can interfere during the critical setup process.

3.  **Multi-threaded Operation (`Example::mainThread` and `Example::videoThread`):**
    -   2 new threads, `mainThread` and `videoThread`, are created to showcase the example.
    -   The application then enters two concurrent loops running in different threads.

4.  **Main Thread Loop (in `Example::mainThread`):**
    -   This loop simulates a primary control thread.
    -   It repeatedly acquires a blocking **exclusive transaction**. This lock guarantees that no other thread can access any device properties until the transaction is finished.
    -   Inside the lock, it reads the current color palette index, calculates the next one, and writes the new value back to the device.
    -   Crucially, it **sleeps for 1 second while holding the lock**. This is done intentionally to make it easy to observe the `videoThread` being blocked, as it will be unable to acquire its own lock during this time.
    -   It then releases the lock and sleeps for 2 seconds before repeating its loop.

5.  **Video Thread Loop (in `Example::videoThread`):**
    -   This loop simulates a background task that continuously processes data.
    -   In each iteration, it attempts to create a **normal properties transaction** using `tryCreatePropertiesTransaction` with a 1-millisecond timeout. This is a non-blocking call.
    -   **If the main thread holds the exclusive lock**, `tryCreatePropertiesTransaction` will fail, and the video thread will log a warning message instead of being stalled. This demonstrates the intended pattern for handling contention.
    -   **If it successfully acquires the lock**, it reads the current shutter temperature from the device.
    -   It then proceeds to read the latest image data from the video stream.
    -   The thread sleeps for 500 milliseconds before its next iteration.

## Properties Operation Modes

The `thermal-core` properties system can be initialized in one of two modes, which determines how property access is handled in a multi-threaded environment. This is configured when calling `core::PropertiesWtc640::createInstance`.

### `core::Properties::Mode::SYNC_DIRECT`

*   **Synchronous Execution:** All operations are executed synchronously on the calling thread. The function will block until the operation is complete.
*   **Use Case:** This mode is ideal for simpler applications, such as command-line tools or scripts, where there is no complex GUI or event loop. It makes the program flow easier to reason about.
*   **Deadlock Avoidance:** As operations are direct function calls, the risk of deadlocking due to a stalled event loop is eliminated. However, care must still be taken to avoid deadlocks from multiple threads trying to acquire locks in conflicting orders.

### `core::Properties::Mode::ASYNC_QUEUED`

*   **Asynchronous Execution:** Operations are placed into a queue and executed by a separate worker thread. This allows the calling thread (e.g., a GUI thread) to remain responsive.
*   **Event Loop Dependency:** This mode is designed for GUI applications that have a running event loop. Certain operations may need to be dispatched back to the main application thread for execution.
*   **Deadlock Risk:** If used in an application without a running event loop on the main thread, this mode can easily cause deadlocks. A common scenario is the main thread calling a function that queues a task back to the main thread and then waits for it to complete. Since the main thread is blocked waiting, it cannot process the queue, and the application stalls. It is recommended to use `SYNC_DIRECT` for non-GUI applications.