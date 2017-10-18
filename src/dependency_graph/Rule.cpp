#include "Rule.hpp"

Rule::Rule(SwitchId switch_id, TableId table_id, uint16_t priority,
           NetworkSpace& domain, std::vector<Action>& action_list):
    switch_id_(switch_id), table_id_(table_id), priority_(priority),
    domain_(domain), action_list_(action_list)
{
    id_ = RuleIdGenerator::getId();

    // DEBUG LOG
    std::cout<<"----- Rule "
             <<id_ <<" at "
             <<switch_id_<<"("<<(int)table_id_<<") H("
             <<domain.header()<<") P("
             <<priority_<<") -> "
             //<<action_list[0].transfer.headerChanger()<<"} P{"
             <<action_list[0].port_id
             <<std::endl;
}

Rule::~Rule()
{
    RuleIdGenerator::releaseId(id_);
}

// TODO: Delete this temporary solution
NetworkSpace Rule::outDomain() const
{
    return action_list_[0].transfer.apply(domain_);
}

Dependency::Dependency(RulePtr _src_rule, RulePtr _dst_rule,
                       NetworkSpace& _domain):
    src_rule(_src_rule),
    dst_rule(_dst_rule),
    domain(_domain)
{
    // DEBUG LOG
    std::cout<<"Dependency "<<src_rule->id()<<"->"<<dst_rule->id()
             <<" ("<<domain.header()<<")"<<std::endl;
}

RuleIdGenerator::RuleIdGenerator()
{
    last_id_ = 1;
}

RuleId RuleIdGenerator::getId()
{
    static RuleIdGenerator generator;
    return generator.last_id_++;
}

void RuleIdGenerator::releaseId(RuleId id)
{
    // TODO: implement release id
}

RuleIterator::RuleIterator(SortedRuleMap::iterator iterator,
                           SortedRuleMap& sorted_rule_map):
    iterator_(iterator),
    sorted_rule_map_(sorted_rule_map)
{
    set_rule_map_iterator();
}
    
bool RuleIterator::operator!=(const RuleIterator& other)
{
    bool this_end = (iterator_ == sorted_rule_map_.end());
    bool other_end = (other.iterator_ == other.sorted_rule_map_.end());

    if (this_end && other_end)
        return false;
    else if (this_end || other_end)
        return true;
    else {
        RulePtr rule = rule_map_iterator_->second;
        RulePtr other_rule = other.rule_map_iterator_->second;
        return (rule->id() != other_rule->id());
    }
}

const RuleIterator& RuleIterator::operator++()
{
    RuleMap& rule_map = iterator_->second;
    rule_map_iterator_++;
    if (rule_map_iterator_ == rule_map.end()) {
        iterator_++;
        set_rule_map_iterator();
    }
}

RulePtr& RuleIterator::operator*() const
{
    return rule_map_iterator_->second;
}

void RuleIterator::set_rule_map_iterator()
{
    while (iterator_ != sorted_rule_map_.end()) {
        RuleMap& rule_map = iterator_->second;
        rule_map_iterator_ = rule_map.begin();
        if (rule_map_iterator_ == rule_map.end()) {
            iterator_++;
        }
        else {
            rule_map_iterator_ = rule_map.begin();
            break;
        }
    }
}

RuleRange::RuleRange(SortedRuleMap& sorted_rule_map):
    sorted_rule_map_(sorted_rule_map),
    begin_(sorted_rule_map_.begin()),
    end_(sorted_rule_map_.end())
{

}


RuleRange::RuleRange(SortedRuleMap& sorted_rule_map,
                     SortedRuleMap::iterator begin,
                     SortedRuleMap::iterator end):
    sorted_rule_map_(sorted_rule_map),
    begin_(begin),
    end_(end)
{

}

RuleIterator RuleRange::begin() const
{
    return {begin_, sorted_rule_map_};
}

RuleIterator RuleRange::end() const
{
    return {end_, sorted_rule_map_};
}