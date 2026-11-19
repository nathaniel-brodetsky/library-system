#include <httplib.h>

#include <charconv>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

#include "library/library_service.hpp"

namespace {

using namespace library;

constexpr std::string_view kSeedIsbn = "978-966-03-4058-9";
constexpr Date kSeedDate = makeDate(2025, 1, 15);

struct ServerOptions {
    std::string host{"0.0.0.0"};
    int port{8080};
    CopyId seedCopies{100'000};
    int workers{128};
};

std::optional<int> toInt(std::string_view text) {
    int value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}

std::string escape(std::string_view text) {
    std::string result;
    for (const char symbol : text) {
        if (symbol == '"' || symbol == '\\') {
            result += '\\';
        }
        result += symbol;
    }
    return result;
}

void reply(httplib::Response& response, int status, const std::string& json) {
    response.status = status;
    response.set_content(json, "application/json; charset=utf-8");
}

void replyError(httplib::Response& response, int status, std::string_view reason) {
    reply(response, status, "{\"error\":\"" + escape(reason) + "\"}");
}

struct RequestFields {
    std::optional<int> reader;
    std::optional<int> copy;
    std::optional<Date> date;
};

RequestFields readLoanFields(const httplib::Request& request) {
    return {toInt(request.get_param_value("reader")), toInt(request.get_param_value("copy")),
            parseDate(request.get_param_value("date"))};
}

std::string_view toString(RegistrationResult result) {
    switch (result) {
        case RegistrationResult::Registered:
            return "Registered";
        case RegistrationResult::InvalidId:
            return "InvalidId";
        case RegistrationResult::EmptyName:
            return "EmptyName";
        case RegistrationResult::InvalidEmail:
            return "InvalidEmail";
        case RegistrationResult::DuplicateId:
            return "DuplicateId";
    }
    return {};
}

std::string_view toString(BorrowResult result) {
    switch (result) {
        case BorrowResult::Issued:
            return "Issued";
        case BorrowResult::ReaderNotRegistered:
            return "ReaderNotRegistered";
        case BorrowResult::OverdueDebt:
            return "OverdueDebt";
        case BorrowResult::LoanLimitReached:
            return "LoanLimitReached";
        case BorrowResult::CopyUnavailable:
            return "CopyUnavailable";
    }
    return {};
}

std::string_view toString(RenewResult result) {
    switch (result) {
        case RenewResult::Renewed:
            return "Renewed";
        case RenewResult::ReaderNotRegistered:
            return "ReaderNotRegistered";
        case RenewResult::NoActiveLoan:
            return "NoActiveLoan";
        case RenewResult::TooEarly:
            return "TooEarly";
        case RenewResult::Overdue:
            return "Overdue";
        case RenewResult::RenewalLimitReached:
            return "RenewalLimitReached";
    }
    return {};
}

std::string_view toString(ReturnOutcome outcome) {
    switch (outcome) {
        case ReturnOutcome::Returned:
            return "Returned";
        case ReturnOutcome::ReaderNotRegistered:
            return "ReaderNotRegistered";
        case ReturnOutcome::CopyNotFound:
            return "CopyNotFound";
        case ReturnOutcome::NoActiveLoan:
            return "NoActiveLoan";
        case ReturnOutcome::InvalidReturnDate:
            return "InvalidReturnDate";
    }
    return {};
}

class LibraryServer {
public:
    explicit LibraryServer(CopyId seedCopies) {
        service_.addBook(Book{std::string{kSeedIsbn}, "Кобзар", "Тарас Шевченко", 1840}, kSeedDate);
        for (CopyId copy = 1; copy <= seedCopies; ++copy) {
            service_.addCopy(BookCopy{copy, std::string{kSeedIsbn}});
        }
    }

    void registerRoutes(httplib::Server& server) {
        server.Get("/health", [](const httplib::Request&, httplib::Response& response) {
            reply(response, 200, "{\"status\":\"ok\"}");
        });
        server.Get("/stats", [this](const httplib::Request&, httplib::Response& response) { stats(response); });
        server.Post("/readers", [this](const httplib::Request& request, httplib::Response& response) {
            registerReader(request, response);
        });
        server.Post("/loans", [this](const httplib::Request& request, httplib::Response& response) {
            borrow(request, response);
        });
        server.Post("/loans/renew", [this](const httplib::Request& request, httplib::Response& response) {
            renew(request, response);
        });
        server.Post("/returns", [this](const httplib::Request& request, httplib::Response& response) {
            giveBack(request, response);
        });
        server.Get(R"(/readers/(\d+)/loans)", [this](const httplib::Request& request, httplib::Response& response) {
            listLoans(request, response);
        });
    }

private:
    void stats(httplib::Response& response) {
        const std::lock_guard lock{mutex_};
        std::ostringstream json;
        json << "{\"readers\":" << service_.readers().size() << ",\"copies\":" << service_.copies().size()
             << ",\"transactions\":" << service_.transactions().size() << '}';
        reply(response, 200, json.str());
    }

    void registerReader(const httplib::Request& request, httplib::Response& response) {
        const std::optional<int> id = toInt(request.get_param_value("id"));
        const std::optional<Date> date = parseDate(request.get_param_value("date"));
        if (!id || !date) {
            replyError(response, 400, "id and date are required");
            return;
        }
        const std::lock_guard lock{mutex_};
        const RegistrationResult result = service_.registerReader(*id, request.get_param_value("name"),
                                                                  request.get_param_value("email"), *date);
        const int status = result == RegistrationResult::Registered    ? 201
                           : result == RegistrationResult::DuplicateId ? 409
                                                                       : 422;
        reply(response, status, "{\"result\":\"" + std::string{toString(result)} + "\"}");
    }

    void borrow(const httplib::Request& request, httplib::Response& response) {
        const RequestFields fields = readLoanFields(request);
        if (!fields.reader || !fields.copy || !fields.date) {
            replyError(response, 400, "reader, copy and date are required");
            return;
        }
        const std::lock_guard lock{mutex_};
        const BorrowReceipt receipt = service_.borrowBook(*fields.reader, *fields.copy, *fields.date);
        if (!receipt.issued()) {
            reply(response, 409, "{\"result\":\"" + std::string{toString(receipt.result)} + "\"}");
            return;
        }
        reply(response, 201,
              "{\"result\":\"Issued\",\"transaction\":" + std::to_string(*receipt.transactionId) + ",\"due\":\"" +
                  formatDate(*receipt.dueDate) + "\"}");
    }

    void renew(const httplib::Request& request, httplib::Response& response) {
        const RequestFields fields = readLoanFields(request);
        if (!fields.reader || !fields.copy || !fields.date) {
            replyError(response, 400, "reader, copy and date are required");
            return;
        }
        const std::lock_guard lock{mutex_};
        const RenewReceipt receipt = service_.renewLoan(*fields.reader, *fields.copy, *fields.date);
        if (!receipt.renewed()) {
            reply(response, 409, "{\"result\":\"" + std::string{toString(receipt.result)} + "\"}");
            return;
        }
        reply(response, 200, "{\"result\":\"Renewed\",\"due\":\"" + formatDate(*receipt.dueDate) + "\"}");
    }

    void giveBack(const httplib::Request& request, httplib::Response& response) {
        const RequestFields fields = readLoanFields(request);
        if (!fields.reader || !fields.copy || !fields.date) {
            replyError(response, 400, "reader, copy and date are required");
            return;
        }
        const BookCondition condition =
            request.get_param_value("condition") == "damaged" ? BookCondition::Damaged : BookCondition::Intact;
        const std::lock_guard lock{mutex_};
        const ReturnReceipt receipt = service_.returnBook(*fields.reader, *fields.copy, *fields.date, condition);
        if (!receipt.returned()) {
            reply(response, 409, "{\"result\":\"" + std::string{toString(receipt.outcome)} + "\"}");
            return;
        }
        reply(response, 200,
              "{\"result\":\"Returned\",\"overdueDays\":" + std::to_string(receipt.fine.overdueDays) +
                  ",\"fine\":" + std::to_string(receipt.fine.total()) + '}');
    }

    void listLoans(const httplib::Request& request, httplib::Response& response) {
        const std::optional<int> reader = toInt(request.matches[1].str());
        if (!reader) {
            replyError(response, 400, "invalid reader id");
            return;
        }
        const std::lock_guard lock{mutex_};
        if (!service_.readers().contains(*reader)) {
            replyError(response, 404, "reader not found");
            return;
        }
        std::ostringstream json;
        json << "{\"reader\":" << *reader << ",\"loans\":[";
        bool first = true;
        for (const Transaction& loan : service_.transactions().openFor(*reader)) {
            json << (first ? "" : ",") << "{\"copy\":" << loan.copyId() << ",\"due\":\"" << formatDate(loan.dueDate())
                 << "\",\"renewals\":" << loan.renewals() << '}';
            first = false;
        }
        json << "]}";
        reply(response, 200, json.str());
    }

    std::mutex mutex_;
    LibraryService service_;
};

ServerOptions parseOptions(int argc, char** argv) {
    ServerOptions options;
    for (int i = 1; i + 1 < argc; i += 2) {
        const std::string_view key{argv[i]};
        const std::string_view value{argv[i + 1]};
        if (key == "--host") {
            options.host = value;
        } else if (key == "--port") {
            options.port = toInt(value).value_or(options.port);
        } else if (key == "--copies") {
            options.seedCopies = toInt(value).value_or(options.seedCopies);
        } else if (key == "--workers") {
            options.workers = toInt(value).value_or(options.workers);
        }
    }
    return options;
}

}  // namespace

int main(int argc, char** argv) {
    const ServerOptions options = parseOptions(argc, argv);
    LibraryServer library{options.seedCopies};
    httplib::Server server;
    const auto workers = static_cast<std::size_t>(options.workers);
    server.new_task_queue = [workers] { return new httplib::ThreadPool(workers); };
    server.set_tcp_nodelay(true);
    library.registerRoutes(server);
    std::cout << "Library server: http://" << options.host << ':' << options.port << " (" << options.seedCopies
              << " copies, " << options.workers << " workers)" << std::endl;
    if (!server.listen(options.host, options.port)) {
        std::cerr << "Cannot listen on port " << options.port << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
