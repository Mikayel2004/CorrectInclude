#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

std::vector<std::string> getFilesInDirectory(const std::string& directoryPath) {
    std::vector<std::string> filenames;
    for (const auto & entry : std::filesystem::directory_iterator(directoryPath)) {
        if (entry.is_regular_file()) {
            filenames.push_back(entry.path().filename().string());
        }
    }
    return filenames;
}

class FilenameMatcher {
private:
    std::unordered_map<std::string, int> filenameToNumberMap;
    std::vector<std::string> numberToFilenameMap;
public:
    FilenameMatcher(const std::string& directoryPath) {
        int number = 0;
        for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".h") {
                std::string filename = entry.path().filename().string();
                filenameToNumberMap[filename] = number;
                numberToFilenameMap.push_back(filename);
                number++;
            }
        }
    }
    int filenameToNumber(const std::string& filename) {
        return filenameToNumberMap[filename];
    }
    std::string numberToFilename(int number) {
        return numberToFilenameMap[number];
    }
};

class DependencyProvider {
    std::map<int, std::vector<int>> dependencies;
public:
    DependencyProvider(const std::string& directoryPath, FilenameMatcher& matcher) {
        std::vector<std::string> filenames = getFilesInDirectory(directoryPath);
        for (const auto& filename : filenames) {
            std::ifstream file(directoryPath + "/" + filename);
            if (!file.is_open()) {
                throw std::runtime_error("Could not open file: " + filename);
            }
            std::string line;
            while (std::getline(file, line)) {
                std::istringstream iss(line);
                std::string dependency;
                while (iss >> dependency) {
                    int fileNumber = matcher.filenameToNumber(filename);
                    int dependencyNumber = matcher.filenameToNumber(dependency);
                    dependencies[fileNumber].push_back(dependencyNumber);
                }
            }
            file.close();
        }
    }
    std::map<int, std::vector<int>> provideDependencies() {
        return dependencies;
    }
};

class OrderValidator {
    std::map<int, std::vector<int>> dependencies;
public:
    OrderValidator(const std::map<int, std::vector<int>>& dependencies) : dependencies(dependencies) {}
    bool isOrderCorrect(const std::vector<std::string>& filenames, FilenameMatcher& matcher) {
        std::vector<int> visited(filenames.size(), 0);
        std::vector<int> recStack(filenames.size(), 0);
        for (const auto& filename : filenames) {
            int fileNumber = matcher.filenameToNumber(filename);
            if (!visited[fileNumber]) {
                if (isCyclic(fileNumber, visited, recStack)) {
                    return false;
                }
            }
        }
        return true;
    }

private:
    bool isCyclic(int v, std::vector<int>& visited, std::vector<int>& recStack) {
        if (!visited[v]) {
            visited[v] = 1;
            recStack[v] = 1;
            for (const auto& i : dependencies[v]) {
                if (!visited[i] && isCyclic(i, visited, recStack)) {
                    return true;
                } else if (recStack[i]) {
                    return true;
                }
            }
        }
        recStack[v] = 0;
        return false;
    }
};

class OrderValidatingEngine {
public:
    void execute(std::istream& in, std::ostream& out) {
        std::string directoryPath;
        std::getline(in, directoryPath);

        FilenameMatcher matcher(directoryPath);
        DependencyProvider provider(directoryPath, matcher);

        std::map<int, std::vector<int>> dependencies = provider.provideDependencies();
        OrderValidator validator(dependencies);

        std::vector<std::string> filenames;
        std::string filename;
        while (std::getline(in, filename)) {
            filenames.push_back(filename);
        }

        if (validator.isOrderCorrect(filenames, matcher)) {
            out << "The order of filenames is correct.\n";
        } else {
            out << "The order of filenames is not correct.\n";
        }
    }
};

int main() {
    OrderValidatingEngine orderValidatingEngine;
    orderValidatingEngine.execute(std::cin, std::cout);
    return 0;
}
