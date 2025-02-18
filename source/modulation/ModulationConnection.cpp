//
// Created by Davis Polito on 12/18/24.
//
#include "common.h"
#include "ModulationConnection.h"
#include <regex>
#include "ModulatorBase.h"
#include "ModulationProcessor.h"
namespace bitklavier
{
    namespace
    {
        const std::string kModulationSourceDelimiter = "_";
        const std::set<std::string> kBipolarModulationSourcePrefixes = {
            "lfo",
            "random",
            "pitch",
            "audio"
        };

        force_inline bool isConnectionAvailable (ModulationConnection* connection)
        {
            return connection->source_name.empty() && connection->destination_name.empty();
        }
    }

    //    ModulationConnection::ModulationConnection(int index, std::string from, std::string to) :
    //                                                                                               source_name(std::move(from)), destination_name(std::move(to)) {
    //        modulation_processor = std::make_unique<ModulationConnectionProcessor>(index);
    //    }

    //    ModulationConnection::~ModulationConnection() { }
    bool ModulationConnection::isModulationSourceDefaultBipolar(const std::string& source) {
        //std::size_t pos = source.find(kModulationSourceDelimiter);

        std::regex pattern(R"(_(.*?)_)");  // Matches text between underscores

        std::smatch match;
        std::regex_search(source,match, pattern);
//        if (std::regex_search(text, match, pattern)) {
//            std::cout << "Found: " << match[1] << std::endl;
//        } else {
//            std::cout << "No match found." << std::endl;
//        }
std::string check = match[1];
        std::size_t pos = check.find_first_of("-");
        std::string prefix = source.substr( 0,pos);
        return kBipolarModulationSourcePrefixes.count(prefix) > 0;
    }



    ModulationConnectionBank::ModulationConnectionBank ()
    {
        for (int i = 0; i < kMaxModulationConnections; ++i)
        {
            std::unique_ptr<ModulationConnection> connection = std::make_unique<ModulationConnection> ("", "", i);
            all_connections_.push_back (std::move (connection));
        }
    }

    ModulationConnectionBank::~ModulationConnectionBank() {}


    ModulationConnection* ModulationConnectionBank::createConnection (const std::string& from, const std::string& to)
    {
        int index = 1;
        for (auto& connection : all_connections_)
        {
            std::string invalid_connection = "modulation_" + std::to_string (index++) + "_amount";
            if (to != invalid_connection && isConnectionAvailable (connection.get()))
            {
                connection->resetConnection (from, to);
                connection->setDefaultBipolar(ModulationConnection::isModulationSourceDefaultBipolar(from));
                return connection.get();
            }
        }

        return nullptr;
    }
    force_inline bool isConnectionAvailable (StateConnection* connection)
    {
        return connection->source_name.empty() && connection->destination_name.empty();
    }

     StateConnectionBank::StateConnectionBank ()
    {
        for (int i = 0; i < kMaxStateConnections; ++i)
        {
            std::unique_ptr<StateConnection> connection = std::make_unique<StateConnection> ("", "", i);
            all_connections_.push_back (std::move (connection));
        }
    }

    StateConnectionBank::~StateConnectionBank() {}


    StateConnection* StateConnectionBank::createConnection (const std::string& from, const std::string& to)
    {
        int index = 1;
        for (auto& connection : all_connections_)
        {
            std::string invalid_connection = "State_" + std::to_string (index++) + "_amount";
            if (to != invalid_connection && isConnectionAvailable (connection.get()))
            {
                connection->resetConnection (from, to);
                return connection.get();
            }
        }

        return nullptr;
    }
}