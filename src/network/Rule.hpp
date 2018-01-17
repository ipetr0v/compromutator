#pragma once

#include "Types.hpp"
#include "Vertex.hpp"
#include "../flow_predictor/Node.hpp"
#include "../openflow/Action.hpp"
#include "../NetworkSpace.hpp"

#include <list>
#include <map>
#include <memory>
#include <vector>

struct PortAction : public PortActionBase
{
    PortAction(PortActionBase&& base, PortPtr port = nullptr):
        PortActionBase(std::move(base)), port(port) {
        // If the port type is normal then the getPort pointer should not be null
        assert(port_type != PortType::NORMAL && port != nullptr);
    }
    PortAction(const PortAction& other) = default;
    PortAction(PortAction&& other) noexcept = default;

    PortPtr port;
};

struct TableAction : public TableActionBase
{
    explicit TableAction(TableActionBase&& base, TablePtr table):
        TableActionBase(std::move(base)), table(table) {
        assert(table == nullptr);
    }
    TableAction(const TableAction& other) = default;
    TableAction(TableAction&& other) noexcept = default;

    TablePtr table;
};

struct GroupAction : public GroupActionBase
{
    GroupAction(GroupActionBase&& base, GroupPtr group):
        GroupActionBase(std::move(base)), group(group) {
        assert(group == nullptr);
    }
    GroupAction(const GroupAction& other) = default;
    GroupAction(GroupAction&& other) noexcept = default;

    GroupPtr group;
};

struct Actions
{
    std::vector<PortAction> port_actions;
    std::vector<TableAction> table_actions;
    std::vector<GroupAction> group_actions;

    bool empty() const {
        return port_actions.empty() &&
               table_actions.empty() &&
               group_actions.empty();
    }
    uint64_t size() const {
        return port_actions.size() +
               table_actions.size() +
               group_actions.size();
    }

    static Actions dropAction() {
        Actions actions;
        actions.port_actions.emplace_back(PortActionBase::dropAction());
        return actions;
    }
    static Actions controllerAction() {
        Actions actions;
        actions.port_actions.emplace_back(PortActionBase::controllerAction());
        return actions;
    }
    static Actions noActions() {
        return Actions();
    }
};

enum class RuleType
{
    FLOW,
    GROUP,
    BUCKET,
    SOURCE,
    SINK
};

struct Descriptor;

class Rule
{
public:
    Rule(RuleType type, SwitchPtr sw, TablePtr table, Priority priority,
         NetworkSpace&& domain, Actions&& actions);
    Rule(const RulePtr other, NetworkSpace&& domain);
    ~Rule();

    RuleType type() const {return type_;}
    RuleId id() const {return id_;}
    SwitchPtr sw() const {return sw_;}
    TablePtr table() const {return table_;}

    Priority priority() const {return priority_;}
    NetworkSpace domain() const {return domain_;}
    PortId inPort() const {return domain_.inPort();}
    const Actions& actions() const {return actions_;}
    uint64_t multiplier() const {return actions_.size();}

    class PtrComparator
    {
    public:
        bool operator()(RulePtr first, RulePtr second) const {
            return first->id_ < second->id_;
        }
    };

private:
    RuleType type_;
    RuleId id_;
    TablePtr table_;
    SwitchPtr sw_;

    Priority priority_;
    NetworkSpace domain_;
    Actions actions_;

    static IdGenerator<uint64_t> id_generator_;

    friend class DependencyGraph;
    friend class PathScan;
    VertexDescriptor vertex_;
    RuleMappingDescriptor rule_mapping_;

};