#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <condition_variable>
#include <chrono>
#include <algorithm>
#include <string>

using namespace std;

class Elevator {
private:
    int currentFloor;
    bool goingUp;
    queue<int> requests;
    mutex mtx;
    condition_variable cv;

    void moveElevator() {
        while (true) {
            unique_lock<mutex> lock(mtx);
            cv.wait(lock, [&]() { return !requests.empty(); });
            lock.unlock();

            // Process all requests before asking for new ones
            while (true) {
                lock_guard<mutex> lock(mtx);
                if (requests.empty()) {
                    break;
                }
                int target = requests.front();
                requests.pop();

                cout << "Moving to floor " << target << "...\n";
                while (currentFloor != target) {
                    this_thread::sleep_for(chrono::seconds(2));
                    currentFloor += goingUp ? 1 : -1;
                    cout << "Elevator at floor " << currentFloor << "\n";
                }

                cout << "Elevator reached floor " << target << "\n";

                if ((goingUp && currentFloor == 9) || (!goingUp && currentFloor == 0)) {
                    goingUp = !goingUp; // Reverse direction at top or bottom floor
                }
            }

            askForNextRequest();
        }
    }

    void askForNextRequest() {
        lock_guard<mutex> lock(mtx);
        string input;
        
        while (true) {
            cout << "Enter floor requests (comma-separated, 1-9): ";
            getline(cin >> ws, input);
            
            bool validInput = true;
            vector<int> newRequests;
            
            // Parse comma-separated input
            size_t pos = 0;
            while (pos < input.length()) {
                size_t nextPos = input.find(',', pos);
                string numStr = input.substr(pos, nextPos - pos);
                
                // Remove whitespace
                numStr.erase(remove_if(numStr.begin(), numStr.end(), ::isspace), numStr.end());
                
                try {
                    int floor = stoi(numStr);
                    if (floor >= 1 && floor <= 9 && floor != currentFloor) {
                        newRequests.push_back(floor);
                    } else {
                        validInput = false;
                        break;
                    }
                } catch (...) {
                    validInput = false;
                    break;
                }
                
                if (nextPos == string::npos) break;
                pos = nextPos + 1;
            }
            
            if (validInput && !newRequests.empty()) {
                // Sort requests based on direction
                sort(newRequests.begin(), newRequests.end(), 
                    [this](int a, int b) {
                        return goingUp ? (a < b) : (a > b);
                    });
                
                // Add all requests to the queue
                for (int floor : newRequests) {
                    requests.push(floor);
                }
                cv.notify_one();
                break;
            } else {
                cout << "Invalid input. Please enter valid floor numbers (1-9).\n";
            }
        }
    }

public:
    Elevator() : currentFloor(0), goingUp(true) {}

    void start() {
        thread elevatorThread(&Elevator::moveElevator, this);
        askForNextRequest();
        elevatorThread.join();
    }
};

int main() {
    cout << "Elevator Simulation with 9 Floors\n";
    cout << "================================\n";
    Elevator elevator;
    elevator.start();
    return 0;
}