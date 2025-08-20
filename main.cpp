#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <cstdint> // For uint32_t

// --- Library Includes (Corrected Paths) ---
#include "imgui.h"
#include "imgui-sfml.h"
#include "portable-file-dialogs.h"

// --- Steganography Logic ---
namespace Steganography {

// Helper to embed a single bit into a color channel
void embedBit(sf::Uint8& colorChannel, bool bit) {
    if (bit) {
        colorChannel |= 1; // Set LSB to 1
    } else {
        colorChannel &= ~1; // Set LSB to 0
    }
}

// Helper to extract a single bit from a color channel
bool extractBit(const sf::Uint8& colorChannel) {
    return (colorChannel & 1);
}

// Main encoding function
std::string encode(const std::string& carrierPath, const std::string& secretPath, const std::string& outputPath) {
    sf::Image carrierImage;
    if (!carrierImage.loadFromFile(carrierPath)) {
        return "Error: Could not load carrier image.";
    }

    std::ifstream secretFile(secretPath, std::ios::binary);
    if (!secretFile) {
        return "Error: Could not open secret file.";
    }

    // Read secret file into a vector
    std::vector<char> secretData((std::istreambuf_iterator<char>(secretFile)), std::istreambuf_iterator<char>());
    uint32_t secretSize = secretData.size();

    // Check if the image has enough capacity
    sf::Vector2u imageSize = carrierImage.getSize();
    uint64_t capacity = (uint64_t)imageSize.x * imageSize.y * 3;
    uint64_t requiredBits = (sizeof(secretSize) * 8) + ((uint64_t)secretSize * 8);

    if (capacity < requiredBits) {
        return "Error: Carrier image is too small to hold the secret data.";
    }

    // --- Embed Data ---
    unsigned int bitIndex = 0;

    // 1. Embed the 32-bit size of the secret file first
    for (int i = 0; i < 32; ++i) {
        sf::Vector2u pos;
        pos.x = (bitIndex / 3) % imageSize.x;
        pos.y = (bitIndex / 3) / imageSize.x;
        sf::Color color = carrierImage.getPixel(pos.x, pos.y);

        bool bit = (secretSize >> i) & 1;

        switch(bitIndex % 3) {
            case 0: embedBit(color.r, bit); break;
            case 1: embedBit(color.g, bit); break;
            case 2: embedBit(color.b, bit); break;
        }

        carrierImage.setPixel(pos.x, pos.y, color);
        bitIndex++;
    }

    // 2. Embed the secret data itself
    for (char byte : secretData) {
        for (int i = 0; i < 8; ++i) {
            sf::Vector2u pos;
            pos.x = (bitIndex / 3) % imageSize.x;
            pos.y = (bitIndex / 3) / imageSize.x;

            sf::Color color = carrierImage.getPixel(pos.x, pos.y);
            bool bit = (byte >> i) & 1;

            switch(bitIndex % 3) {
                case 0: embedBit(color.r, bit); break;
                case 1: embedBit(color.g, bit); break;
                case 2: embedBit(color.b, bit); break;
            }

            carrierImage.setPixel(pos.x, pos.y, color);
            bitIndex++;
        }
    }

    if (!carrierImage.saveToFile(outputPath)) {
        return "Error: Failed to save the output image. Ensure it's a .png file.";
    }

    return "Success! Data encoded and saved to " + outputPath;
}

// Main decoding function
std::string decode(const std::string& stegoPath, const std::string& outputPath) {
    sf::Image stegoImage;
    if (!stegoImage.loadFromFile(stegoPath)) {
        return "Error: Could not load the steganographic image.";
    }

    sf::Vector2u imageSize = stegoImage.getSize();
    unsigned int bitIndex = 0;

    // 1. Extract the 32-bit size of the secret file
    uint32_t secretSize = 0;
    for (int i = 0; i < 32; ++i) {
        sf::Vector2u pos;
        pos.x = (bitIndex / 3) % imageSize.x;
        pos.y = (bitIndex / 3) / imageSize.x;
        sf::Color color = stegoImage.getPixel(pos.x, pos.y);
        bool bit = false;

        switch(bitIndex % 3) {
            case 0: bit = extractBit(color.r); break;
            case 1: bit = extractBit(color.g); break;
            case 2: bit = extractBit(color.b); break;
        }

        if (bit) {
            secretSize |= (1 << i);
        }
        bitIndex++;
    }

    // Sanity check
    uint64_t capacity = (uint64_t)imageSize.x * imageSize.y * 3;
    if ((uint64_t)secretSize * 8 + 32 > capacity) {
        return "Error: Decoded size is invalid or larger than image capacity.";
    }
    if (secretSize == 0) {
        return "Warning: Decoded size is 0. Nothing to extract.";
    }

    // 2. Extract the secret data
    std::vector<char> secretData;
    secretData.reserve(secretSize);
    for (uint32_t byte_i = 0; byte_i < secretSize; ++byte_i) {
        char currentByte = 0;
        for (int bit_i = 0; bit_i < 8; ++bit_i) {
            sf::Vector2u pos;
            pos.x = (bitIndex / 3) % imageSize.x;
            pos.y = (bitIndex / 3) / imageSize.x;

            sf::Color color = stegoImage.getPixel(pos.x, pos.y);
            bool bit = false;

            switch(bitIndex % 3) {
                case 0: bit = extractBit(color.r); break;
                case 1: bit = extractBit(color.g); break;
                case 2: bit = extractBit(color.b); break;
            }

            if (bit) {
                currentByte |= (1 << bit_i);
            }
            bitIndex++;
        }
        secretData.push_back(currentByte);
    }

    std::ofstream outputFile(outputPath, std::ios::binary);
    if (!outputFile) {
        return "Error: Could not create output file for decoded data.";
    }
    outputFile.write(secretData.data(), secretData.size());
    outputFile.close();

    return "Success! Decoded data saved to " + outputPath;
}

} // namespace Steganography

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 450), "Steganography Tool", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);

    // Buffers for file paths
    char carrierPath[256] = "";
    char secretPath[256] = "";
    char encodeOutputPath[256] = "output.png";
    char stegoPath[256] = "";
    char decodeOutputPath[256] = "decoded_file";
    char status[256] = "Ready.";

    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(window, event);
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::SetNextWindowSize(ImVec2(800, 450));
        ImGui::SetNextWindowPos(ImVec2(0,0));
        ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        // --- ENCODING UI ---
        ImGui::Text("--- Encode ---");
        ImGui::InputText("Carrier Image", carrierPath, 256, ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        if (ImGui::Button("...##1")) {
             auto f = pfd::open_file("Select a carrier image", ".", {"Image Files", "*.png *.bmp"}).result();
             if (!f.empty()) strncpy(carrierPath, f[0].c_str(), 256);
        }

        ImGui::InputText("Secret File", secretPath, 256, ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        if (ImGui::Button("...##2")) {
            auto f = pfd::open_file("Select a secret file").result();
            if (!f.empty()) strncpy(secretPath, f[0].c_str(), 256);
        }

        ImGui::InputText("Output Image Path", encodeOutputPath, 256);

        if (ImGui::Button("Encode")) {
            std::string result = Steganography::encode(carrierPath, secretPath, encodeOutputPath);
            strncpy(status, result.c_str(), 256);
        }

        ImGui::Separator();

        // --- DECODING UI ---
        ImGui::Text("--- Decode ---");
        ImGui::InputText("Stego Image", stegoPath, 256, ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        if (ImGui::Button("...##3")) {
            auto f = pfd::open_file("Select a stego image", ".", {"Image Files", "*.png *.bmp"}).result();
            if (!f.empty()) strncpy(stegoPath, f[0].c_str(), 256);
        }

        ImGui::InputText("Decoded File Path", decodeOutputPath, 256);

        if (ImGui::Button("Decode")) {
            std::string result = Steganography::decode(stegoPath, decodeOutputPath);
            strncpy(status, result.c_str(), 256);
        }

        ImGui::Separator();

        // --- STATUS ---
        ImGui::Text("Status:");
        ImGui::TextWrapped("%s", status);

        ImGui::End();

        window.clear();
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}