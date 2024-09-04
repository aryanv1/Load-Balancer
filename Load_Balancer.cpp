#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <string>

using namespace std;

enum class RequestType {
    TYPE_A,
    TYPE_B,
};

class Destination {
public:
    string ipAddress;
    int requestsBeingServed = 0;
    int threshold;

    Destination(const string& ip, int thresh) : ipAddress(ip), threshold(thresh) {}

    bool acceptRequest(const Request& request) {
        if (requestsBeingServed < threshold) {
            requestsBeingServed++;
            return true;
        } else {
            return false;
        }
    }

    void completeRequest() {
        requestsBeingServed--;
    }
};

class Service {
public:
    string name;
    unordered_set<Destination*> destinations;

    void addDestination(Destination* destination) {
        destinations.insert(destination);
    }

    void removeDestination(Destination* destination) {
        destinations.erase(destination);
    }
};

class Request {
public:
    string id;
    RequestType requestType;
    unordered_map<string, string> parameters;
};

class LoadBalancer {
protected:
    unordered_map<RequestType, Service*> serviceMap;

public:
    void registerService(RequestType requestType, Service* service) {
        serviceMap[requestType] = service;
    }

    unordered_set<Destination*> getDestinations(const Request& request) {
        return serviceMap[request.requestType]->destinations;
    }

    virtual Destination* balanceLoad(const Request& request) = 0;
};

class LeastConnectionLoadBalancer : public LoadBalancer {
public:
    Destination* balanceLoad(const Request& request) override;
};

Destination* LeastConnectionLoadBalancer::balanceLoad(const Request& request) {
    auto destinations = getDestinations(request);
    auto minElem = min_element(destinations.begin(), destinations.end(),
        [](Destination* a, Destination* b) {
            return a->requestsBeingServed < b->requestsBeingServed;
        });

    if (minElem != destinations.end()) {
        return *minElem;
    } else {
        throw runtime_error("No destination available");
    }
}

class RoutedLoadBalancer : public LoadBalancer {
public:
    Destination* balanceLoad(const Request& request) override;
};

Destination* RoutedLoadBalancer::balanceLoad(const Request& request) {
    auto destinations = getDestinations(request);
    vector<Destination*> list(destinations.begin(), destinations.end());
    return list[hash<string>{}(request.id) % list.size()];
}

class RoundRobinLoadBalancer : public LoadBalancer {
    unordered_map<RequestType, queue<Destination*>> destinationsForRequest;

public:
    Destination* balanceLoad(const Request& request) override;

private:
    queue<Destination*> convertToQueue(const unordered_set<Destination*>& destinations);
};

Destination* RoundRobinLoadBalancer::balanceLoad(const Request& request) {
    if (!destinationsForRequest.contains(request.requestType)) {
        auto destinations = getDestinations(request);
        destinationsForRequest[request.requestType] = convertToQueue(destinations);
    }
    auto destination = destinationsForRequest[request.requestType].front();
    destinationsForRequest[request.requestType].pop();
    destinationsForRequest[request.requestType].push(destination);
    return destination;
}

queue<Destination*> RoundRobinLoadBalancer::convertToQueue(const unordered_set<Destination*>& destinations) {
    queue<Destination*> q;
    for (auto& d : destinations) {
        q.push(d);
    }
    return q;
}

int main() {
    // Create different types of load balancers
    LeastConnectionLoadBalancer leastConnectionLB;
    RoundRobinLoadBalancer roundRobinLB;

    // Create a service and destinations
    Service service;
    service.name = "Example Service";

    Destination destination1("192.168.1.1", 5);
    Destination destination2("192.168.1.2", 5);

    service.addDestination(&destination1);
    service.addDestination(&destination2);

    // Register the service with the load balancers
    leastConnectionLB.registerService(RequestType::TYPE_A, &service);
    roundRobinLB.registerService(RequestType::TYPE_A, &service);

    // Create a request
    Request request{"12345", RequestType::TYPE_A, {{"key", "value"}}};

    // Balance the load using different strategies
    Destination* selectedDestination1 = leastConnectionLB.balanceLoad(request);
    Destination* selectedDestination2 = roundRobinLB.balanceLoad(request);

    // Output selected destinations
    cout << "Least Connection selected: " << selectedDestination1->ipAddress << endl;
    cout << "Round Robin selected: " << selectedDestination2->ipAddress << endl;

    return 0;
}