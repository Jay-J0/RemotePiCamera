#include <zmq.hpp>             // ZeroMQ for messaging
#include <opencv2/opencv.hpp>  // OpenCV for video capture and processing
#include <iostream>
#include <vector>
#include <thread>              // For sleep function

int main() {
    zmq::context_t context(1);  // Initialize ZeroMQ context with a single I/O thread

    // Create a publisher socket to send video stream
    zmq::socket_t pub_socket(context, ZMQ_PUB);
    // Create an XPUB socket to detect client subscriptions
    zmq::socket_t xpub_socket(context, ZMQ_XPUB);

    // Bind publisher socket to TCP port 5555 (for sending frames)
    pub_socket.bind("tcp://*:5555");

    // Bind XPUB socket to TCP port 5556 (for detecting client subscriptions)
    xpub_socket.bind("tcp://*:5556");

    std::cout << "Server started. Waiting for client to connect...\n";

    while (true) {
        zmq::message_t subscription_msg;

        // Block execution until a client subscribes (detects first client connection)
        xpub_socket.recv(subscription_msg, zmq::recv_flags::none);

        std::cout << "Client connected! Starting webcam...\n";

        // Open the default webcam (device ID 0)
        cv::VideoCapture cap(0);
        if (!cap.isOpened()) {
            std::cerr << "Error: Could not open webcam\n";
            continue;
        }

        while (true) {
            cv::Mat frame;
            cap >> frame;  // Capture a frame from the webcam

            // If the frame is empty, log an error and continue
            if (frame.empty()) {
                std::cerr << "Error: Captured empty frame! Skipping...\n";
                continue;
            }

            // Encode the frame as JPEG before sending
            std::vector<uchar> buf;
            if (!cv::imencode(".jpg", frame, buf)) {
                std::cerr << "Error: Frame encoding failed! Skipping...\n";
                continue;
            }

            // Create a ZeroMQ message containing the encoded image
            zmq::message_t message(buf.data(), buf.size());

            // Send the image to all subscribed clients
            pub_socket.send(message, zmq::send_flags::none);

            // Check if the client is still connected by reading from XPUB socket (non-blocking)
            zmq::message_t check_msg;
            if (xpub_socket.recv(check_msg, zmq::recv_flags::dontwait)) {
                std::cout << "Client disconnected. Stopping webcam...\n";
                cap.release();  // Release the webcam
                break;  // Exit the inner loop and wait for a new client
            }

            // Add a small delay (30ms) to control the frame rate
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
    }

    return 0;
}
