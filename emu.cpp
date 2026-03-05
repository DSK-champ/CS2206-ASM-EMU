#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>

using namespace std;

int main(int argc, char* argv[]) {
    // Ensure a filename argument was provided
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }

    // Open file in binary mode, seeking to end to determine file size
    ifstream file(argv[1], ios::binary | ios::ate);
    if (!file) {
        cerr << "Error: cannot open file '" << argv[1] << "'\n";
        return 1;
    }

    // Get the total size of the file in bytes
    streamsize size = file.tellg();
    file.seekg(0, ios::beg); // Seek back to the beginning before reading

    // Allocate a buffer to hold the entire file contents
    vector<char> buffer(size);

    // Read all bytes from the file into the buffer
    if (!file.read(buffer.data(), size)) {
        cerr << "Error: failed to read file\n";
        return 1;
    }

    cout << "Successfully read " << size << " bytes from '" << argv[1] << "'\n\n";

    // Print a hex dump of the file contents (16 bytes per row)
    for (streamsize i = 0; i < size; ++i) {

        // Print the byte offset at the start of each row
        if (i % 16 == 0)
            cout << hex << setw(8) << setfill('0') << i << "  ";

        // Print each byte as a 2-digit hex value
        cout << hex << setw(2) << setfill('0')
             << (static_cast<unsigned int>(buffer[i]) & 0xFF) << " ";

        // Print a newline after every 16 bytes or at end of file
        if ((i + 1) % 16 == 0 || i + 1 == size)
            cout << "\n";
    }

    return 0; // File is automatically closed when ifstream goes out of scope
}