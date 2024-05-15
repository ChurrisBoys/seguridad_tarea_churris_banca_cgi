#include <iostream>
#include <string>

using namespace std;

int main() {
    cout << "Content-type: text/plain\n\n";  // Changed to plain text for simplicity

    // Read POST data: assuming input is directly from stdin
    string input;
    getline(cin, input); // Reads one line from stdin, modify if expecting more data

    // Optional: Print received data to debug (not required for production)
    cerr << "Received data: " << input << endl;

    // Send a specific response back
    cout << "helloooooooo " << input << endl;

    return 0;
}

