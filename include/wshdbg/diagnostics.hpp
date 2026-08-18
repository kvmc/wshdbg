#pragma once

#include <string>
#include <vector>

namespace wshdbg {

enum class DiagnosticLevel {
    Off,
    Error,
    Debug,
    Trace
};

struct DiagnosticEvent {
    DiagnosticLevel level{DiagnosticLevel::Error};
    std::string category;
    std::string message;
    std::string timestamp;
};

class DiagnosticBuffer {
public:
    void add(DiagnosticEvent event) {
        events_.push_back(std::move(event));
    }

    const std::vector<DiagnosticEvent>& events() const {
        return events_;
    }

    void clear() {
        events_.clear();
    }

private:
    std::vector<DiagnosticEvent> events_;
};

}
