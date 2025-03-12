#include <opencv2/opencv.hpp>  // OpenCV for image processing
#include <zmq.hpp>             // ZeroMQ for messaging
#include <iostream>

int main() {
    zmq::context_t context(1);  // Initialize ZeroMQ context with a single I/O thread

    // Create a subscriber socket to receive video stream from the server
    zmq::socket_t subscribe_socket(context, ZMQ_SUB);
    subscribe_socket.connect("tcp://x.x.x.x:5555");  // Connect to the publisher (server ip)
    subscribe_socket.set(zmq::sockopt::subscribe, ""); // Subscribe to all messages

    // Additional subscriber socket to XPUB for server start detection
    zmq::socket_t xsub_socket(context, ZMQ_SUB);
    xsub_socket.connect("tcp://x.x.x.x:5556");  // Connect to the XPUB socket (server ip)
    xsub_socket.set(zmq::sockopt::subscribe, ""); // Subscribe to all messages

    std::cout << "Connected to server. Receiving video stream...\n";

    while (true) {
        zmq::message_t message;

        // Receive a message (image frame) from the server
        if (!subscribe_socket.recv(message, zmq::recv_flags::none)) {
            std::cout << "Server disconnected. Exiting...\n";
            break;
        }

        // If the message is empty, log a warning and skip this frame
        if (message.size() == 0) {
            std::cerr << "Warning: Received empty frame, skipping...\n";
            continue;
        }

        // Convert received raw data into a vector of unsigned chars
        std::vector<uchar> buf(static_cast<uchar*>(message.data()), 
                               static_cast<uchar*>(message.data()) + message.size());

        // Decode the received JPEG image into an OpenCV Mat object
        cv::Mat frame = cv::imdecode(buf, cv::IMREAD_COLOR);

        // If the decoding fails, log a warning and skip this frame
        if (frame.empty()) {
            std::cerr << "Warning: Decoded frame is empty, skipping...\n";
            continue;
        }

        // Display the received frame in a window
        cv::imshow("Live Stream", frame);

        // Exit the loop if the ESC key (ASCII 27) is pressed
        if (cv::waitKey(1) == 27) break;
    }

    std::cout << "Disconnected from server.\n";
    return 0;
}



