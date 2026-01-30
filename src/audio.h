#ifndef AUDIO_H
#define AUDIO_H

#include <M5Cardputer.h>
#include <SD.h>
#include <SPI.h>
#include <vector>

// SD Card pins for Cardputer ADV
constexpr int SD_SCK  = 40;
constexpr int SD_MISO = 39;
constexpr int SD_MOSI = 14;
constexpr int SD_CS   = 12;

constexpr uint8_t MAX_SAMPLES = 16;  // Max samples we can load

// WAV header structure
struct WavHeader {
    char riff[4];
    uint32_t fileSize;
    char wave[4];
    char fmt[4];
    uint32_t fmtSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
};

// Sample buffer
struct Sample {
    int16_t* data = nullptr;
    size_t length = 0;
    uint32_t sampleRate = 22050;
    bool loaded = false;
    char name[16] = {0};  // Short name for display
};

class AudioManager {
public:
    Sample samples[MAX_SAMPLES];
    uint8_t sampleCount = 0;
    std::vector<String> wavFiles;  // List of WAV files on SD
    bool sdInitialized = false;

    bool init() {
        // Initialize SD card with custom SPI pins
        SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

        if (!SD.begin(SD_CS, SPI, 25000000)) {
            Serial.println("SD Card init failed!");
            return false;
        }

        sdInitialized = true;
        Serial.println("SD Card initialized");

        // Set speaker volume
        M5Cardputer.Speaker.setVolume(200);

        // Scan for WAV files
        scanWavFiles();

        return true;
    }

    void scanWavFiles() {
        wavFiles.clear();
        File root = SD.open("/");
        if (!root) return;

        while (true) {
            File entry = root.openNextFile();
            if (!entry) break;

            if (entry.isDirectory()) {
                entry.close();
                continue;
            }

            String fullPath = entry.name();  // Returns full path like "/1.wav"
            entry.close();

            // Get just the filename part
            int lastSlash = fullPath.lastIndexOf('/');
            String filename = (lastSlash >= 0) ? fullPath.substring(lastSlash + 1) : fullPath;

            // Skip macOS resource fork files (start with ._)
            if (filename.startsWith("._")) {
                Serial.printf("Skipping macOS file: %s\n", fullPath.c_str());
                continue;
            }

            // Check if it's a WAV file
            if (filename.endsWith(".wav") || filename.endsWith(".WAV")) {
                // Ensure path starts with /
                String path = fullPath;
                if (!path.startsWith("/")) {
                    path = "/" + path;
                }
                wavFiles.push_back(path);
                Serial.printf("Added WAV: %s\n", path.c_str());
            }
        }
        root.close();

        // Sort files alphabetically
        std::sort(wavFiles.begin(), wavFiles.end());

        Serial.printf("Total WAV files found: %d\n", wavFiles.size());
    }

    uint8_t getWavFileCount() {
        return wavFiles.size();
    }

    const char* getWavFileName(uint8_t index) {
        if (index < wavFiles.size()) {
            return wavFiles[index].c_str();
        }
        return "";
    }

    // Get short name for display (without path and extension)
    String getShortName(uint8_t index) {
        if (index >= wavFiles.size()) return "---";
        String name = wavFiles[index];
        // Remove leading /
        if (name.startsWith("/")) name = name.substring(1);
        // Remove .wav extension
        int dotPos = name.lastIndexOf('.');
        if (dotPos > 0) name = name.substring(0, dotPos);
        // Truncate if too long
        if (name.length() > 8) name = name.substring(0, 8);
        return name;
    }

    bool loadSample(uint8_t index, const char* filename) {
        Serial.printf("loadSample(%d, %s)\n", index, filename);

        if (index >= MAX_SAMPLES || !sdInitialized) {
            Serial.printf("Cannot load: idx=%d sd=%d\n", index, sdInitialized);
            return false;
        }

        File file = SD.open(filename, FILE_READ);
        if (!file) {
            Serial.printf("Failed to open: %s\n", filename);
            // Try without leading slash
            if (filename[0] == '/') {
                file = SD.open(filename + 1, FILE_READ);
                if (!file) {
                    Serial.printf("Also failed: %s\n", filename + 1);
                    return false;
                }
            } else {
                return false;
            }
        }

        Serial.printf("File opened, size=%d\n", file.size());

        // Read WAV header
        WavHeader header;
        file.read((uint8_t*)&header, sizeof(WavHeader));

        // Validate WAV format
        if (memcmp(header.riff, "RIFF", 4) != 0 ||
            memcmp(header.wave, "WAVE", 4) != 0) {
            Serial.printf("%s is not a valid WAV file\n", filename);
            file.close();
            return false;
        }

        // Debug: print WAV info
        Serial.printf("WAV: %s - %dHz %dbit %dch\n",
            filename, header.sampleRate, header.bitsPerSample, header.numChannels);

        // Skip to data chunk
        if (header.fmtSize > 16) {
            file.seek(file.position() + (header.fmtSize - 16));
        }

        // Find data chunk
        char chunkId[4];
        uint32_t dataSize = 0;
        while (file.available()) {
            file.read((uint8_t*)chunkId, 4);
            file.read((uint8_t*)&dataSize, 4);

            if (memcmp(chunkId, "data", 4) == 0) {
                break;
            }
            file.seek(file.position() + dataSize);
        }

        if (dataSize == 0) {
            Serial.printf("No data chunk in %s\n", filename);
            file.close();
            return false;
        }

        // Calculate samples
        size_t bytesPerSample = header.bitsPerSample / 8;
        size_t numSamples = dataSize / bytesPerSample;

        // Free existing data
        if (samples[index].data != nullptr) {
            free(samples[index].data);
            samples[index].data = nullptr;
        }

        // Allocate memory (PSRAM preferred)
        samples[index].data = (int16_t*)ps_malloc(numSamples * sizeof(int16_t));
        if (!samples[index].data) {
            samples[index].data = (int16_t*)malloc(numSamples * sizeof(int16_t));
        }

        if (!samples[index].data) {
            Serial.printf("Memory allocation failed for %s\n", filename);
            file.close();
            return false;
        }

        // Read audio data
        if (header.bitsPerSample == 16) {
            file.read((uint8_t*)samples[index].data, dataSize);
        } else if (header.bitsPerSample == 8) {
            uint8_t* temp = (uint8_t*)malloc(dataSize);
            if (temp) {
                file.read(temp, dataSize);
                for (size_t i = 0; i < numSamples; i++) {
                    samples[index].data[i] = (int16_t)((temp[i] - 128) << 8);
                }
                free(temp);
            }
        }

        file.close();

        samples[index].length = numSamples;
        samples[index].sampleRate = header.sampleRate;
        samples[index].loaded = true;

        // Store short name
        String shortName = filename;
        if (shortName.startsWith("/")) shortName = shortName.substring(1);
        int dotPos = shortName.lastIndexOf('.');
        if (dotPos > 0) shortName = shortName.substring(0, dotPos);
        strncpy(samples[index].name, shortName.c_str(), 15);

        Serial.printf("Loaded %s: %d samples @ %dHz\n",
                      filename, numSamples, header.sampleRate);

        if (index >= sampleCount) sampleCount = index + 1;

        return true;
    }

    void playSample(uint8_t index, uint8_t channel = 0) {
        if (index >= MAX_SAMPLES || !samples[index].loaded) {
            Serial.printf("playSample: index %d not loaded\n", index);
            return;
        }

        Serial.printf("Playing sample %d on ch %d\n", index, channel);
        M5Cardputer.Speaker.playRaw(
            samples[index].data,
            samples[index].length,
            samples[index].sampleRate,
            false,
            1,
            channel % 4,  // Use channels 0-3
            true
        );
    }

    void setVolume(uint8_t volume) {
        M5Cardputer.Speaker.setVolume(volume);
    }

    void stopAll() {
        M5Cardputer.Speaker.stop();
    }
};

#endif
