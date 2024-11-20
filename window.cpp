//
// Created by mk on 10/28/24.
//

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include "tinyfiledialogs.h"
#include <algorithm>
#include <thread>
#include <chrono>

#include <unistd.h>

const std::string mediaDir = "media";
const std::string mediaFilePath = mediaDir + "/directories.txt";
const std::string iconPath = "/icons";
const std::string playIconPath = iconPath + "/play.png";
const std::string pauseIconPath = iconPath + "/pause.png";
const std::string nextIconPath = iconPath + "/next.png";
const std::string prevIconPath = iconPath + "/previous.png";

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

std::string findProjectRoot() {
    std::filesystem::path currentPath = std::filesystem::current_path();
    while (!std::filesystem::exists(currentPath / "CMakeLists.txt")) {
        if (currentPath.has_parent_path()) {
            currentPath = currentPath.parent_path();
        } else {
            throw std::runtime_error("Project root not found");
        }
    }
    return currentPath.string();
}

std::string toLowerCase(const std::string& str) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
}

void saveMediaPaths(const std::vector<std::string>& paths){
    std::ofstream outFile(mediaFilePath);
    if (outFile.is_open()){
        for(const auto& path : paths) {
            outFile << path << std::endl;
        }
        outFile.close();
    } else {
        std::cerr << "Could not open file for writing: " << mediaFilePath << std::endl;
    }
}

std::vector<std::string> loadMediaPaths() {
    std::vector<std::string> paths;
    std::ifstream inFile(mediaFilePath);
    std::string line;

    while(std::getline(inFile, line)) {
        paths.push_back(line);
    }
    return paths;
}

void clearMediaPaths(std::vector<std::string>& mediaPaths) {
    // Clear the contents of the directories.txt file
    std::ofstream outFile(mediaFilePath, std::ofstream::out | std::ofstream::trunc);
    if (outFile.is_open()) {
        outFile.close();
    } else {
        std::cerr << "Could not open file for writing: " << mediaFilePath << std::endl;
    }
    mediaPaths.clear();
}

void addAudioFilesFromDirectory(const std::string& directory, std::vector<std::string>& selectedFiles) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            if (ext == ".mp3" || ext == ".wav" || ext == ".ogg" || ext == ".flac" || ext == ".aac") {
                std::cout << "Adding: " << entry.path().string() << std::endl;
                selectedFiles.push_back(entry.path().string());
            }
        }
    }
}

std::vector<std::string> splitPaths(const std::string& input, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(input);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

std::vector<std::string> openFileDialog(sf::RenderWindow& window) {
    std::vector<std::string> selectedFiles;

    const char* filters[] = {"*.mp3", "*.wav", "*.ogg", "*.flac", "*.aac"};
    const char* paths = tinyfd_openFileDialog("Select Audio Files", "", 5, filters, nullptr, 1);
    std::cout << paths << std::endl;
    if (paths) {
        std::string path(paths);
        std::vector<std::string> individualPaths = splitPaths(path, '|');
        for(const auto& subpath : individualPaths) {
            if (std::filesystem::is_directory(subpath)) {
                addAudioFilesFromDirectory(subpath, selectedFiles);
            } else {
                selectedFiles.push_back(subpath);
            }
        }
    }

    return selectedFiles;
}

std::vector<std::string> openFolderDialog(sf::RenderWindow& window) {
    std::vector<std::string> selectedFiles;
    const char* folderPath = tinyfd_selectFolderDialog("Select Folder", "");

    if (folderPath) {
        addAudioFilesFromDirectory(folderPath, selectedFiles);
    }
    return selectedFiles;
}


void openSettingsWindow() {
    sf::RenderWindow settingsWindow(sf::VideoMode(400, 300), "Settings");
    sf::RectangleShape applyButton(sf::Vector2f(100,40));
    applyButton.setFillColor(sf::Color::Blue);
    applyButton.setPosition(150, 200);

    sf::Font font;
    if(!font.loadFromFile("arial.ttf")) {
        if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
            std::cerr << "Could not load font\n";
            return;
        }
    }

    sf::Text buttonText("Apply", font, 20);
    buttonText.setFillColor(sf::Color::White);
    buttonText.setPosition(170, 210);

    while (settingsWindow.isOpen()) {
        sf::Event event;
        while(settingsWindow.pollEvent(event)) {
            if(event.type == sf::Event::Closed) {
                settingsWindow.close();
            }
            if(event.type == sf::Event::MouseButtonPressed) {
                if (applyButton.getGlobalBounds().contains(event.mouseButton.x, event.mouseButton.y)) {
                    std::cout << "Settings applied!" << std::endl;
                    settingsWindow.close();
                }
            }

        }
        settingsWindow.clear(sf::Color(50,50,50));
        settingsWindow.draw(applyButton);
        settingsWindow.draw(buttonText);
        settingsWindow.display();
    }
}

sf::Music music;
std::vector<std::string> mediaQueue;
size_t currentMediaIndex = 0;
int selectedMediaIndex = -1;
bool isPlaying = false;
bool isDraggingVolume = false;
bool isDraggingSlider = false;
float volumeOffset = 0.0f;
float sliderOffset = 0.0f;

sf::Texture playTexture, pauseTexture, nextTexture, prevTexture;
sf::Sprite playButtonSprite, nextButtonSprite, prevButtonSprite;


void playMedia(const std::string& mediaPath){
    if(music.openFromFile(mediaPath)) {
        music.play();
        isPlaying = true;
    } else {
        std::cerr << "Could not play media: " << mediaPath << std::endl;
    }
}

void stopMedia() {
    music.stop();
    isPlaying = false;
}

void nextMedia() {
    if(mediaQueue.empty()) return;
    currentMediaIndex = (currentMediaIndex + 1) % mediaQueue.size();
    selectedMediaIndex = currentMediaIndex;
    stopMedia();
    std::cout << "Next Media: " << mediaQueue[currentMediaIndex] << std::endl;
    playMedia(mediaQueue[currentMediaIndex]);
}

void prevMedia() {
    if(mediaQueue.empty()) return;
    stopMedia();
    currentMediaIndex = (currentMediaIndex == 0) ? mediaQueue.size() - 1 : currentMediaIndex - 1;
    selectedMediaIndex = currentMediaIndex;
    playMedia(mediaQueue[currentMediaIndex]);
}

void openMainWindow() {
    if(!std::filesystem::exists(mediaDir)){
        std::filesystem::create_directory(mediaDir);
    }

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        std::cout << "Current working directory: " << cwd << std::endl;
    } else {
        perror("getcwd() error");
    }

    std::string projectRoot = findProjectRoot();
    std::cout << projectRoot << std::endl;

    if (!playTexture.loadFromFile(projectRoot + playIconPath) ||
        !pauseTexture.loadFromFile(projectRoot + pauseIconPath) ||
        !nextTexture.loadFromFile(projectRoot + nextIconPath) ||
        !prevTexture.loadFromFile(projectRoot + prevIconPath)){
        std::cerr << "Error loading button textures!" << std::endl;
        std::cout << "Attempting to load image: " << projectRoot + playIconPath << std::endl;
        return;
    }

    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Aecros", sf::Style::Default);
    sf::View view = window.getDefaultView();
    sf::Font font;

    if(!font.loadFromFile("arial.ttf")) {
        if(!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
            std::cerr << "Could not load font\n";
            return;
        }
    }

    std::string searchQuery = "";

    playButtonSprite.setTexture(playTexture);
    nextButtonSprite.setTexture(nextTexture);
    prevButtonSprite.setTexture(prevTexture);

    playButtonSprite.setPosition(100, WINDOW_HEIGHT-40);
    nextButtonSprite.setPosition(150, WINDOW_HEIGHT-40);
    prevButtonSprite.setPosition(50, WINDOW_HEIGHT-40);

    playButtonSprite.setScale(0.05f, 0.05f);
    nextButtonSprite.setScale(0.05f, 0.05f);
    prevButtonSprite.setScale(0.05f, 0.05f);

    sf::RectangleShape navBar(sf::Vector2f(WINDOW_WIDTH, 30));
    navBar.setFillColor(sf::Color(60,60,60));

    sf::RectangleShape footer(sf::Vector2f(WINDOW_WIDTH, 50));
    footer.setFillColor(sf::Color(60, 60, 60));
    footer.setPosition(0, WINDOW_HEIGHT-50);

    sf::RectangleShape sliderBar(sf::Vector2f(400, 10));
    sliderBar.setFillColor(sf::Color(80, 80, 80));
    sliderBar.setPosition(200, WINDOW_HEIGHT - 35);

    sf::RectangleShape sliderKnob(sf::Vector2f(10, 20));
    sliderKnob.setFillColor(sf::Color::White);
    sliderKnob.setPosition(200, WINDOW_HEIGHT - 40);

    sf::RectangleShape volumeBar(sf::Vector2f(60, 10));
    volumeBar.setFillColor(sf::Color(80, 80, 80));
    volumeBar.setPosition(620, WINDOW_HEIGHT-35);

    sf::RectangleShape volumeKnob(sf::Vector2f(10,20));
    volumeKnob.setFillColor(sf::Color::White);
    volumeKnob.setPosition(620+50, WINDOW_HEIGHT-40);

    int FILE_MENU_SIZE_LENGTH = 150;
    int FILE_MENU_ITEM_HEIGHT = 30;

    sf::RectangleShape fileMenu(sf::Vector2f(FILE_MENU_SIZE_LENGTH, FILE_MENU_ITEM_HEIGHT));
    fileMenu.setFillColor(sf::Color(100, 100, 100));
    fileMenu.setPosition(10,0);

    sf::RectangleShape settingsOption(sf::Vector2f(FILE_MENU_SIZE_LENGTH, FILE_MENU_ITEM_HEIGHT));
    settingsOption.setFillColor(sf::Color(120, 120, 120));
    settingsOption.setPosition(10, 30);

    sf::RectangleShape importMediaDropdownButton(sf::Vector2f(FILE_MENU_SIZE_LENGTH, FILE_MENU_ITEM_HEIGHT));
    importMediaDropdownButton.setFillColor(sf::Color(120, 120, 120));
    importMediaDropdownButton.setPosition(10, 60);

    sf::RectangleShape importMediaFolderButton(sf::Vector2f(FILE_MENU_SIZE_LENGTH, FILE_MENU_ITEM_HEIGHT));
    importMediaFolderButton.setFillColor(sf::Color(120, 120, 120));
    importMediaFolderButton.setPosition(10, 90);

    sf::RectangleShape clearMediaButton(sf::Vector2f(FILE_MENU_SIZE_LENGTH, FILE_MENU_ITEM_HEIGHT));
    clearMediaButton.setFillColor(sf::Color(120, 120, 120));
    clearMediaButton.setPosition(10, 120);

    sf::Text fileMenuText("File", font, 15);
    fileMenuText.setFillColor(sf::Color::White);
    fileMenuText.setPosition(20, 5);

    sf::Text settingsText("Settings", font, 15);
    settingsText.setFillColor(sf::Color::White);
    settingsText.setPosition(20,35);

    sf::Text importMediaDropdownText("Import Media File(s)", font, 15);
    importMediaDropdownText.setFillColor(sf::Color::White);
    importMediaDropdownText.setPosition(20, 65);

    sf::Text importMediaFolderText("Import Media Folder", font, 15);
    importMediaFolderText.setFillColor(sf::Color::White);
    importMediaFolderText.setPosition(20, 95);

    sf::Text clearMediaText("Clear Media", font, 15);
    clearMediaText.setFillColor(sf::Color::White);
    clearMediaText.setPosition(20, 125);

    sf::RectangleShape searchBar(sf::Vector2f(200, 24));
    searchBar.setFillColor(sf::Color(80, 80, 80));
    searchBar.setPosition(WINDOW_WIDTH-210, 3);

    sf::Text searchText(searchQuery, font, 15);
    searchText.setFillColor(sf::Color::White);
    searchText.setPosition(WINDOW_WIDTH-200, 5);

    sf::Text volumeLevelText(std::to_string(static_cast<int>(music.getVolume())) + "%", font, 15);
    volumeLevelText.setFillColor(sf::Color::White);
    volumeLevelText.setPosition(volumeKnob.getPosition().x + 20, volumeKnob.getPosition().y); // Slightly offset from the knob
    window.draw(volumeLevelText);


    std::vector<std::string> mediaPaths = loadMediaPaths();
    bool noMediaDetected = mediaPaths.empty();
    bool dropdownVisible = false;

    while (window.isOpen()) {
        sf::Event event;

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode < 128) {
                    if (event.text.unicode == 8 && !searchQuery.empty()) {
                        searchQuery.pop_back();
                    } else {
                        searchQuery += static_cast<char>(event.text.unicode);
                    }
                    searchText.setString(searchQuery);
                }
            }

            if (event.type == sf::Event::Resized) {
                // Update the view to the new size
                view.setSize(event.size.width, event.size.height);  // Set view size to new window size
                view.setCenter(event.size.width / 2.0f, event.size.height / 2.0f);  // Keep view centered
                window.setView(view);

                // Resize UI elements (buttons, sliders, etc.)
//                float scaleX = static_cast<float>(event.size.width) / WINDOW_WIDTH;
//                float scaleY = static_cast<float>(event.size.height) / WINDOW_HEIGHT;
                float scaleX = 1.0f; //place holder will need to fix
                float scaleY = 1.0f;

                playButtonSprite.setPosition(100 * scaleX, (WINDOW_HEIGHT - 40) * scaleY);
                nextButtonSprite.setPosition(150 * scaleX, (WINDOW_HEIGHT - 40) * scaleY);
                prevButtonSprite.setPosition(50 * scaleX, (WINDOW_HEIGHT - 40) * scaleY);

                sliderBar.setPosition(200 * scaleX, (WINDOW_HEIGHT - 35) * scaleY);
                sliderKnob.setPosition(200 * scaleX, (WINDOW_HEIGHT - 40) * scaleY);

                volumeBar.setPosition(620 * scaleX, (WINDOW_HEIGHT - 35) * scaleY);
                volumeKnob.setPosition((620 + 50) * scaleX, (WINDOW_HEIGHT - 40) * scaleY);

                navBar.setSize(sf::Vector2f(event.size.width, 30));
                footer.setSize(sf::Vector2f(event.size.width, 50));
                footer.setPosition(0, event.size.height - 50);

                searchBar.setPosition(event.size.width - 210, 3);
                searchText.setFillColor(sf::Color::White);
                searchText.setPosition(event.size.width-200, 5);


                // Resize other UI elements similarly
            }


            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            // File menu hover
            if(fileMenu.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                fileMenu.setFillColor(sf::Color(80, 80, 80));
            } else {
                fileMenu.setFillColor(sf::Color(100, 100, 100));
            }

            // Settings menu hover
            if (dropdownVisible && settingsOption.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                settingsOption.setFillColor(sf::Color(140, 140, 140));
            } else {
                settingsOption.setFillColor(sf::Color(120, 120, 120));
            }

            // Import Dropdown hover
            if (dropdownVisible && importMediaDropdownButton.getGlobalBounds().contains(mousePos.x, mousePos.y)){
                importMediaDropdownButton.setFillColor(sf::Color(140, 140, 140));
            } else {
                importMediaDropdownButton.setFillColor(sf::Color(120, 120, 120));
            }

            if (dropdownVisible && importMediaFolderButton.getGlobalBounds().contains(mousePos.x, mousePos.y)){
                importMediaFolderButton.setFillColor(sf::Color(140, 140, 140));
            } else {
                importMediaFolderButton.setFillColor(sf::Color(120, 120, 120));
            }

            if (dropdownVisible && clearMediaButton.getGlobalBounds().contains(mousePos.x, mousePos.y)){
                clearMediaButton.setFillColor(sf::Color(140, 140, 140));
            } else {
                clearMediaButton.setFillColor(sf::Color(120, 120, 120));
            }


            if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);

                if(playButtonSprite.getGlobalBounds().contains(event.mouseButton.x, event.mouseButton.y)) {
                    if (isPlaying) {
                        music.pause();
                        isPlaying = false;
                        playButtonSprite.setTexture(playTexture);
                    } else {
                        music.play();
                        isPlaying = true;
                        playButtonSprite.setTexture(pauseTexture);
                    }
                }

                if (sliderKnob.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                    isDraggingSlider = true;
                    sliderOffset = mousePos.x - sliderKnob.getPosition().x;
                }
                // Check if we're dragging the volume knob
                if (volumeKnob.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                    isDraggingVolume = true;
                    volumeOffset = mousePos.x - volumeKnob.getPosition().x;
                }

                if(nextButtonSprite.getGlobalBounds().contains(event.mouseButton.x, event.mouseButton.y)){
                    nextMedia();
                }

                if(prevButtonSprite.getGlobalBounds().contains(event.mouseButton.x, event.mouseButton.y)){
                    prevMedia();
                }

                size_t yOffset = 50;
                for (size_t i = 0; i < mediaPaths.size(); ++i) {
                    sf::Text mediaText(mediaPaths[i], font, 15);
                    mediaText.setPosition(100, yOffset);

                    // Check if mouse clicked on a media path (song)
                    if (mediaText.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                        selectedMediaIndex = i;  // Store the index of the selected media
                        playMedia(mediaPaths[i]); // Play the selected media
                        mediaQueue = mediaPaths;
                        currentMediaIndex = i;
                        for(std::string media: mediaPaths) {
                            std::cout << media << std::endl;
                        }
                        playButtonSprite.setTexture(pauseTexture);
                    }
                    yOffset += 30;
                }

                if(fileMenu.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                    dropdownVisible = !dropdownVisible;
                }

                if (dropdownVisible && importMediaDropdownButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                    std::vector<std::string> files = openFileDialog(window);
                    mediaPaths.insert(mediaPaths.end(), files.begin(), files.end());
                    saveMediaPaths(mediaPaths); // Save updated paths
                    noMediaDetected = mediaPaths.empty(); // Update the status
                    dropdownVisible = false;
                }

                if (dropdownVisible && importMediaFolderButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                    std::vector<std::string> files = openFolderDialog(window);
                    mediaPaths.insert(mediaPaths.end(), files.begin(), files.end());
                    saveMediaPaths(mediaPaths); // Save updated paths
                    noMediaDetected = mediaPaths.empty(); // Update the status
                    dropdownVisible = false;
                }


                if(dropdownVisible && settingsOption.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                    openSettingsWindow();
                    dropdownVisible = false;
                }

                if(dropdownVisible && clearMediaButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                    clearMediaPaths(mediaPaths);
                    dropdownVisible = false;
                    //std::cout << mediaPaths.empty();
                    noMediaDetected = mediaPaths.empty();
                }
            }

            if (event.type == sf::Event::MouseButtonReleased) {
                isDraggingSlider = false;
                isDraggingVolume = false;
            }

            if (isDraggingSlider) {
                float newPosX = std::max(sliderBar.getPosition().x, std::min((float)mousePos.x - sliderOffset, sliderBar.getPosition().x + sliderBar.getSize().x - sliderKnob.getSize().x));

                sliderKnob.setPosition(newPosX, sliderKnob.getPosition().y);

                // Calculate new playback position based on slider position
                float progress = (newPosX - sliderBar.getPosition().x) / sliderBar.getSize().x;
                music.setPlayingOffset(sf::seconds(music.getDuration().asSeconds() * progress));
            }

            if (isDraggingVolume) {
                float newPosX = std::max(volumeBar.getPosition().x, std::min((float)mousePos.x - volumeOffset, volumeBar.getPosition().x + volumeBar.getSize().x - volumeKnob.getSize().x));

                volumeKnob.setPosition(newPosX, volumeKnob.getPosition().y);

                // Calculate new volume based on knob position
                float volume = 100.0f * ((newPosX - volumeBar.getPosition().x) / volumeBar.getSize().x);
                music.setVolume(volume);  // Set volume in range 0-100
            }


        }

        if (isPlaying && !isDraggingSlider) {
            float progress = music.getPlayingOffset().asSeconds() / music.getDuration().asSeconds();
            sliderKnob.setPosition(sliderBar.getPosition().x + sliderBar.getSize().x * progress, sliderKnob.getPosition().y);
        }

        window.clear(sf::Color::Black);
        window.setView(view);

        window.draw(navBar);
        window.draw(footer);
        window.draw(playButtonSprite);
        window.draw(nextButtonSprite);
        window.draw(prevButtonSprite);
        window.draw(sliderBar);
        window.draw(sliderKnob);
        window.draw(volumeBar);
        window.draw(volumeKnob);
        window.draw(fileMenu);
        window.draw(fileMenuText);
        window.draw(searchBar);
        window.draw(searchText);
        if(dropdownVisible) {
            window.draw(settingsOption);
            window.draw(settingsText);
            window.draw(importMediaDropdownButton);
            window.draw(importMediaFolderButton);
            window.draw(importMediaFolderText);
            window.draw(importMediaDropdownText);
            window.draw(clearMediaButton);
            window.draw(clearMediaText);
        }
        if (noMediaDetected) {
            sf::Text noMediaText("No media detected!", font, 15);
            noMediaText.setFillColor(sf::Color::White);
            noMediaText.setPosition((WINDOW_WIDTH-120)/2, (WINDOW_HEIGHT+20)/2 - 20);
            window.draw(noMediaText);
        } else {
            // Vector to store only matching media paths
            std::vector<std::string> matchingMediaPaths;
            std::string lowerSearchQuery = toLowerCase(searchQuery); // Convert search query to lowercase

            // Collect only the matching media paths
            for (const auto& mediaPath : mediaPaths) {
                std::string lowerMediaPath = toLowerCase(mediaPath); // Convert media path to lowercase
                if (lowerMediaPath.find(lowerSearchQuery) != std::string::npos) {
                    matchingMediaPaths.push_back(mediaPath); // Add to matching list
                }
            }

            // If there are no matching items, display a "No matches" text
            if (matchingMediaPaths.empty() && !searchQuery.empty()) {
                sf::Text noMatchesText("No matches found!", font, 15);
                noMatchesText.setFillColor(sf::Color::White);
                noMatchesText.setPosition((WINDOW_WIDTH - 120) / 2, (WINDOW_HEIGHT + 20) / 2 - 20);
                window.draw(noMatchesText);
            } else {
                // Draw only matching media paths
                size_t yOffset = 50; // Starting Y position
                for (size_t i = 0; i < matchingMediaPaths.size(); ++i) {
                    sf::Text mediaText(matchingMediaPaths[i], font, 15);
                    mediaText.setFillColor(sf::Color::White);
                    mediaText.setPosition(100, yOffset);
                    if (i == selectedMediaIndex) {
                        mediaText.setFillColor(sf::Color::Green);
                    }
                    window.draw(mediaText);
                    yOffset += 30; // Increment Y position for the next item
                }
            }
        }

        window.display();
    }
}