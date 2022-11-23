#include <iostream>
#include <fstream>
// #include <boost/functional/hash.hpp>
#include <regex>
#include <unordered_set>
#include <set>
#include <list>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>
#include <queue>

#define SYMBOLS 0
#define INITIAL 1
#define GOAL 2
#define ACTIONS 3
#define ACTION_DEFINITION 4
#define ACTION_PRECONDITION 5
#define ACTION_EFFECT 6

class GroundedCondition;
class Condition;
class GroundedAction;
class Action;
class Env;

using namespace std;

bool print_status = true;

class GroundedCondition
{
private:
    string predicate;
    list<string> arg_values;
    bool truth = true;

public:
    GroundedCondition(string predicate, list<string> arg_values, bool truth = true)
    {
        this->predicate = predicate;
        this->truth = truth;  // fixed
        for (string l : arg_values)
        {
            this->arg_values.push_back(l);
        }
    }

    GroundedCondition(const GroundedCondition& gc)
    {
        this->predicate = gc.predicate;
        this->truth = gc.truth;  // fixed
        for (string l : gc.arg_values)
        {
            this->arg_values.push_back(l);
        }
    }

    string get_predicate() const
    {
        return this->predicate;
    }
    list<string> get_arg_values() const
    {
        return this->arg_values;
    }

    bool get_truth() const
    {
        return this->truth;
    }

    friend ostream& operator<<(ostream& os, const GroundedCondition& pred)
    {
        os << pred.toString() << " ";
        return os;
    }

    bool operator==(const GroundedCondition& rhs) const
    {
        if (this->predicate != rhs.predicate || this->arg_values.size() != rhs.arg_values.size())
            return false;

        auto lhs_it = this->arg_values.begin();
        auto rhs_it = rhs.arg_values.begin();

        while (lhs_it != this->arg_values.end() && rhs_it != rhs.arg_values.end())
        {
            if (*lhs_it != *rhs_it)
                return false;
            ++lhs_it;
            ++rhs_it;
        }

        if (this->truth != rhs.get_truth()) // fixed
            return false;

        return true;
    }

    string toString() const
    {
        string temp = "";
        temp += this->predicate;
        temp += "(";
        for (string l : this->arg_values)
        {
            temp += l + ",";
        }
        temp = temp.substr(0, temp.length() - 1);
        temp += ")";
        return temp;
    }

  
};

struct GroundedConditionComparator
{
    bool operator()(const GroundedCondition& lhs, const GroundedCondition& rhs) const
    {
        return lhs == rhs;
    }
};

struct GroundedConditionHasher
{
    size_t operator()(const GroundedCondition& gcond) const
    {
        return hash<string>{}(gcond.toString());
    }
};

class Condition
{
private:
    string predicate;
    list<string> args;
    bool truth;

public:
    Condition(string pred, list<string> args, bool truth)
    {
        this->predicate = pred;
        this->truth = truth;
        for (string ar : args)
        {
            this->args.push_back(ar);
        }
    }

    string get_predicate() const
    {
        return this->predicate;
    }

    list<string> get_args() const
    {
        return this->args;
    }

    bool get_truth() const
    {
        return this->truth;
    }

    friend ostream& operator<<(ostream& os, const Condition& cond)
    {
        os << cond.toString() << " ";
        return os;
    }

    bool operator==(const Condition& rhs) const // fixed
    {

        if (this->predicate != rhs.predicate || this->args.size() != rhs.args.size())
            return false;

        auto lhs_it = this->args.begin();
        auto rhs_it = rhs.args.begin();

        while (lhs_it != this->args.end() && rhs_it != rhs.args.end())
        {
            if (*lhs_it != *rhs_it)
                return false;
            ++lhs_it;
            ++rhs_it;
        }

        if (this->truth != rhs.get_truth())
            return false;

        return true;
    }

    string toString() const
    {
        string temp = "";
        if (!this->truth)
            temp += "!";
        temp += this->predicate;
        temp += "(";
        for (string l : this->args)
        {
            temp += l + ",";
        }
        temp = temp.substr(0, temp.length() - 1);
        temp += ")";
        return temp;
    }

};

struct ConditionComparator
{
    bool operator()(const Condition& lhs, const Condition& rhs) const
    {
        return lhs == rhs;
    }
};

struct ConditionHasher
{
    size_t operator()(const Condition& cond) const
    {
        return hash<string>{}(cond.toString());
    }
};

class Action
{
private:
    string name;
    list<string> args;
    unordered_set<Condition, ConditionHasher, ConditionComparator> preconditions;
    unordered_set<Condition, ConditionHasher, ConditionComparator> effects;
    
    //added
    int num_args; 

    //used for evaluating preconditions/effects
    unordered_map<string,string> symbol_map;


public:
    Action(string name, list<string> args,
        unordered_set<Condition, ConditionHasher, ConditionComparator>& preconditions,
        unordered_set<Condition, ConditionHasher, ConditionComparator>& effects,
        unordered_set<string> symbols)
    {
        this->name = name;
        for (string l : args)
        {
            this->args.push_back(l);
            this->symbol_map[l] = l; //adding in action-specific symbols
        }
        for (Condition pc : preconditions)
        {
            this->preconditions.insert(pc);
            // this->preconditions_tracker.push_back(false);
        }
        for (Condition pc : effects)
        {
            this->effects.insert(pc);
        }
        
        // printf("Enrolling original symbols\n");
        // printf("Original: \t Remapped: \n");

        for(string symbol: symbols)
        {
            symbol_map[symbol] = symbol; // adding in environmental symbols for mapping preconditions/effects
            // cout<<symbol<<"\t"<<symbol_map[symbol]<<endl;
        }

        this->num_args = args.size();
        
    }

    string get_name() const
    {
        return this->name;
    }
    list<string> get_args() const
    {
        return this->args;
    }
    unordered_set<Condition, ConditionHasher, ConditionComparator> get_preconditions() const
    {
        return this->preconditions;
    }
    unordered_set<Condition, ConditionHasher, ConditionComparator> get_effects() const
    {
        return this->effects;
    }

    bool operator==(const Action& rhs) const
    {
        if (this->get_name() != rhs.get_name() || this->get_args().size() != rhs.get_args().size())
            return false;

        return true;
    }

    friend ostream& operator<<(ostream& os, const Action& ac)
    {
        os << ac.toString() << endl;
        os << "Precondition: ";
        for (Condition precond : ac.get_preconditions())
            os << precond;
        os << endl;
        os << "Effect: ";
        for (Condition effect : ac.get_effects())
            os << effect;
        os << endl;
        return os;
    }

    string toString() const
    {
        string temp = "";
        temp += this->get_name();
        temp += "(";
        for (string l : this->get_args())
        {
            temp += l + ",";
        }
        temp = temp.substr(0, temp.length() - 1);
        temp += ")";
        return temp;
    }
    
    //generates an updated symbol map given arguments
    unordered_map<string,string> generate_symbol_map(list<string> input_args)
    {
        //create temporary copy of map
        unordered_map<string,string> ret_symbol_map = symbol_map;

        // update values in map to reflect input args (use temporary map)
        auto input_iter = input_args.begin();
        for(auto arg_iter = args.begin(); arg_iter != args.end(); arg_iter++)
        {
            // cout<< "replacing " <<  temp_symbol_map[arg_iter->data()] << " with " << input_iter->data() << endl; 
            ret_symbol_map[arg_iter->data()] = input_iter->data(); //looks up key by arguments, and updates with the value of the input args 
            // cout<< "    validating " <<  arg_iter->data() << " -> " << temp_symbol_map[arg_iter->data()] << endl; 
            input_iter++;
        }
        return ret_symbol_map;
    }

    //takes input args and updates the corresponding values in the list using a symbol map
    list<string> get_remapped_args(list<string> original_args, unordered_map<string,string> sym_map)
    {
        list<string> ret_list; 
        for(auto o_arg : original_args)
        {
            ret_list.push_back(sym_map[o_arg]); //adds the new mapping to the return list
        }

        return ret_list;
    }

    void print_umap(unordered_map<string,string> sym_map)
    {
        printf("Remapped Arguments: \n");
        printf("\t Original: \t Remapped: \n");
        for(auto iter = sym_map.begin(); iter != sym_map.end(); iter++)
        {
            cout<<"\t"<< iter->first <<"\t\t"<< iter->second <<endl;
        }
    }

    int get_num_args()
    {
        return this->num_args;
    }

    //will need to make an ungrounded version of this
    bool preconditions_satisfied( unordered_set<Condition, ConditionHasher, ConditionComparator> beginning_state, list<string> input_args)
    {
        
        unordered_map<string,string> temp_symbol_map = generate_symbol_map(input_args);
        // print_umap(temp_symbol_map);
        
        // iterate over preconditions to determine whether the remapped version is present. If it is, continue searching, if not return false.
        for(Condition pc: preconditions)
        {
            Condition temp_condition = Condition(pc.get_predicate(),this->get_remapped_args(pc.get_args(), temp_symbol_map) , pc.get_truth());
            // cout<<"search condition: " << temp_condition.toString() << endl;
            if(beginning_state.find(temp_condition) == beginning_state.end())
            {
                return false;
            }
        }
        return true;
    }

    //runs action and returns ending conditions
    unordered_set<Condition, ConditionHasher, ConditionComparator> execute_action(
        unordered_set<Condition, ConditionHasher, ConditionComparator> beginning_state, 
        list<string> input_args)
        {
            //create copy of start condition that will be modified and returned
            unordered_set<Condition, ConditionHasher, ConditionComparator> end_conditions = beginning_state; 

            //remap effect conditions with inputs
            unordered_map<string,string> temp_symbol_map = generate_symbol_map(input_args);
            // print_umap(temp_symbol_map);

            //update end_conditions with the effects
            for(Condition effect : effects)
            {
                if(effect.get_truth()) //if adding a condition
                {
                    Condition temp_condition = Condition(effect.get_predicate(),this->get_remapped_args(effect.get_args(), temp_symbol_map), effect.get_truth());
                    end_conditions.insert(temp_condition);
                }
                else //if removing a condition
                {
                    Condition temp_condition = Condition(effect.get_predicate(),this->get_remapped_args(effect.get_args(), temp_symbol_map), true);
                    end_conditions.erase(temp_condition);
                }
            }

            return end_conditions;
        }

};

struct ActionComparator
{
    bool operator()(const Action& lhs, const Action& rhs) const
    {
        return lhs == rhs;
    }
};

struct ActionHasher
{
    size_t operator()(const Action& ac) const
    {
        return hash<string>{}(ac.get_name());
    }
};

class Env
{
private:
    unordered_set<GroundedCondition, GroundedConditionHasher, GroundedConditionComparator> initial_conditions;
    unordered_set<GroundedCondition, GroundedConditionHasher, GroundedConditionComparator> goal_conditions;
    unordered_set<Action, ActionHasher, ActionComparator> actions;
    unordered_set<string> symbols;

public:
    void remove_initial_condition(GroundedCondition gc)
    {
        this->initial_conditions.erase(gc);
    }
    void add_initial_condition(GroundedCondition gc)
    {
        this->initial_conditions.insert(gc);
    }
    void add_goal_condition(GroundedCondition gc)
    {
        this->goal_conditions.insert(gc);
    }
    void remove_goal_condition(GroundedCondition gc)
    {
        this->goal_conditions.erase(gc);
    }
    void add_symbol(string symbol)
    {
        symbols.insert(symbol);
        // sym_map[symbol] = symbol;
    }
    void add_symbols(list<string> symbols)
    {
        for (string l : symbols)
            this->symbols.insert(l);
    }
    void add_action(Action action)
    {
        this->actions.insert(action);
    }

    Action get_action(string name)
    {
        for (Action a : this->actions)
        {
            if (a.get_name() == name)
                return a;
        }
        throw runtime_error("Action " + name + " not found!");
    }

    unordered_set<Action, ActionHasher, ActionComparator> get_actions() const 
    {
        return this->actions;
    }

    unordered_set<GroundedCondition, GroundedConditionHasher, GroundedConditionComparator> get_initial_conditions() const
    {
        return this->initial_conditions;
    }
    
    unordered_set<GroundedCondition, GroundedConditionHasher, GroundedConditionComparator> get_goal_conditions() const
    {
        return this->goal_conditions;
    }

    unordered_set<Condition, ConditionHasher, ConditionComparator> get_initial_ungrounded()
    {
        unordered_set<Condition, ConditionHasher, ConditionComparator> ret_val; 

        for(auto ground_it:initial_conditions)
        {
            Condition temp_cond = Condition(ground_it.get_predicate(), ground_it.get_arg_values(), ground_it.get_truth());
            ret_val.insert(temp_cond);
        }

        return ret_val;
    }

    unordered_set<Condition, ConditionHasher, ConditionComparator> get_goal_ungrounded()
    {
        unordered_set<Condition, ConditionHasher, ConditionComparator> ret_val; 

        for(auto ground_it:goal_conditions)
        {
            Condition temp_cond = Condition(ground_it.get_predicate(), ground_it.get_arg_values(), ground_it.get_truth());
            ret_val.insert(temp_cond);
        }
        return ret_val;
    }

    unordered_set<string> get_symbols() const
    {
        return this->symbols;
    }

    friend ostream& operator<<(ostream& os, const Env& w)
    {
        os << "***** Environment *****" << endl << endl;
        os << "Symbols: ";
        for (string s : w.get_symbols())
            os << s + ",";
        os << endl;
        os << "Initial conditions: ";
        for (GroundedCondition s : w.initial_conditions)
            os << s;
        os << endl;
        os << "Goal conditions: ";
        for (GroundedCondition g : w.goal_conditions)
            os << g;
        os << endl;
        os << "Actions:" << endl;
        for (Action g : w.actions)
            os << g << endl;
        cout << "***** Environment Created! *****" << endl;
        return os;
    }
};

class GroundedAction
{
private:
    string name;
    list<string> arg_values;

public:
    GroundedAction(string name, list<string> arg_values)
    {
        this->name = name;
        for (string ar : arg_values)
        {
            this->arg_values.push_back(ar);
        }
    }

    string get_name() const
    {
        return this->name;
    }

    list<string> get_arg_values() const
    {
        return this->arg_values;
    }

    bool operator==(const GroundedAction& rhs) const
    {
        if (this->name != rhs.name || this->arg_values.size() != rhs.arg_values.size())
            return false;

        auto lhs_it = this->arg_values.begin();
        auto rhs_it = rhs.arg_values.begin();

        while (lhs_it != this->arg_values.end() && rhs_it != rhs.arg_values.end())
        {
            if (*lhs_it != *rhs_it)
                return false;
            ++lhs_it;
            ++rhs_it;
        }
        return true;
    }

    friend ostream& operator<<(ostream& os, const GroundedAction& gac)
    {
        os << gac.toString() << " ";
        return os;
    }

    string toString() const
    {
        string temp = "";
        temp += this->name;
        temp += "(";
        for (string l : this->arg_values)
        {
            temp += l + ",";
        }
        temp = temp.substr(0, temp.length() - 1);
        temp += ")";
        return temp;
    }
};

list<string> parse_symbols(string symbols_str)
{
    list<string> symbols;
    size_t pos = 0;
    string delimiter = ",";
    while ((pos = symbols_str.find(delimiter)) != string::npos)
    {
        string symbol = symbols_str.substr(0, pos);
        symbols_str.erase(0, pos + delimiter.length());
        symbols.push_back(symbol);
    }
    symbols.push_back(symbols_str);
    return symbols;
}

Env* create_env(char* filename)
{
    ifstream input_file(filename);
    Env* env = new Env();
    regex symbolStateRegex("symbols:", regex::icase);
    regex symbolRegex("([a-zA-Z0-9_, ]+) *");
    regex initialConditionRegex("initialconditions:(.*)", regex::icase);
    regex conditionRegex("(!?[A-Z][a-zA-Z_]*) *\\( *([a-zA-Z0-9_, ]+) *\\)");
    regex goalConditionRegex("goalconditions:(.*)", regex::icase);
    regex actionRegex("actions:", regex::icase);
    regex precondRegex("preconditions:(.*)", regex::icase);
    regex effectRegex("effects:(.*)", regex::icase);
    int parser = SYMBOLS;

    unordered_set<Condition, ConditionHasher, ConditionComparator> preconditions;
    unordered_set<Condition, ConditionHasher, ConditionComparator> effects;
    string action_name;
    string action_args;

    string line;
    if (input_file.is_open())
    {
        while (getline(input_file, line))
        {
            string::iterator end_pos = remove(line.begin(), line.end(), ' ');
            line.erase(end_pos, line.end());

            if (line == "")
                continue;

            if (parser == SYMBOLS)
            {
                smatch results;
                if (regex_search(line, results, symbolStateRegex))
                {
                    line = line.substr(8);
                    sregex_token_iterator iter(line.begin(), line.end(), symbolRegex, 0);
                    sregex_token_iterator end;

                    env->add_symbols(parse_symbols(iter->str()));  // fixed

                    parser = INITIAL;
                }
                else
                {
                    cout << "Symbols are not specified correctly." << endl;
                    throw;
                }
            }
            else if (parser == INITIAL)
            {
                const char* line_c = line.c_str();
                if (regex_match(line_c, initialConditionRegex))
                {
                    const std::vector<int> submatches = { 1, 2 };
                    sregex_token_iterator iter(
                        line.begin(), line.end(), conditionRegex, submatches);
                    sregex_token_iterator end;

                    while (iter != end)
                    {
                        // name
                        string predicate = iter->str();
                        iter++;
                        // args
                        string args = iter->str();
                        iter++;

                        if (predicate[0] == '!')
                        {
                            env->remove_initial_condition(
                                GroundedCondition(predicate.substr(1), parse_symbols(args)));
                        }
                        else
                        {
                            env->add_initial_condition(
                                GroundedCondition(predicate, parse_symbols(args)));
                        }
                    }

                    parser = GOAL;
                }
                else
                {
                    cout << "Initial conditions not specified correctly." << endl;
                    throw;
                }
            }
            else if (parser == GOAL)
            {
                const char* line_c = line.c_str();
                if (regex_match(line_c, goalConditionRegex))
                {
                    const std::vector<int> submatches = { 1, 2 };
                    sregex_token_iterator iter(
                        line.begin(), line.end(), conditionRegex, submatches);
                    sregex_token_iterator end;

                    while (iter != end)
                    {
                        // name
                        string predicate = iter->str();
                        iter++;
                        // args
                        string args = iter->str();
                        iter++;

                        if (predicate[0] == '!')
                        {
                            env->remove_goal_condition(
                                GroundedCondition(predicate.substr(1), parse_symbols(args)));
                        }
                        else
                        {
                            env->add_goal_condition(
                                GroundedCondition(predicate, parse_symbols(args)));
                        }
                    }

                    parser = ACTIONS;
                }
                else
                {
                    cout << "Goal conditions not specified correctly." << endl;
                    throw;
                }
            }
            else if (parser == ACTIONS)
            {
                const char* line_c = line.c_str();
                if (regex_match(line_c, actionRegex))
                {
                    parser = ACTION_DEFINITION;
                }
                else
                {
                    cout << "Actions not specified correctly." << endl;
                    throw;
                }
            }
            else if (parser == ACTION_DEFINITION)
            {
                const char* line_c = line.c_str();
                if (regex_match(line_c, conditionRegex))
                {
                    const std::vector<int> submatches = { 1, 2 };
                    sregex_token_iterator iter(
                        line.begin(), line.end(), conditionRegex, submatches);
                    sregex_token_iterator end;
                    // name
                    action_name = iter->str();
                    iter++;
                    // args
                    action_args = iter->str();
                    iter++;

                    parser = ACTION_PRECONDITION;
                }
                else
                {
                    cout << "Action not specified correctly." << endl;
                    throw;
                }
            }
            else if (parser == ACTION_PRECONDITION)
            {
                const char* line_c = line.c_str();
                if (regex_match(line_c, precondRegex))
                {
                    const std::vector<int> submatches = { 1, 2 };
                    sregex_token_iterator iter(
                        line.begin(), line.end(), conditionRegex, submatches);
                    sregex_token_iterator end;

                    while (iter != end)
                    {
                        // name
                        string predicate = iter->str();
                        iter++;
                        // args
                        string args = iter->str();
                        iter++;

                        bool truth;

                        if (predicate[0] == '!')
                        {
                            predicate = predicate.substr(1);
                            truth = false;
                        }
                        else
                        {
                            truth = true;
                        }

                        Condition precond(predicate, parse_symbols(args), truth);
                        preconditions.insert(precond);
                    }

                    parser = ACTION_EFFECT;
                }
                else
                {
                    cout << "Precondition not specified correctly." << endl;
                    throw;
                }
            }
            else if (parser == ACTION_EFFECT)
            {
                const char* line_c = line.c_str();
                if (regex_match(line_c, effectRegex))
                {
                    const std::vector<int> submatches = { 1, 2 };
                    sregex_token_iterator iter(
                        line.begin(), line.end(), conditionRegex, submatches);
                    sregex_token_iterator end;

                    while (iter != end)
                    {
                        // name
                        string predicate = iter->str();
                        iter++;
                        // args
                        string args = iter->str();
                        iter++;

                        bool truth;

                        if (predicate[0] == '!')
                        {
                            predicate = predicate.substr(1);
                            truth = false;
                        }
                        else
                        {
                            truth = true;
                        }

                        Condition effect(predicate, parse_symbols(args), truth);
                        effects.insert(effect);
                    }

                    env->add_action(
                        Action(action_name, parse_symbols(action_args), preconditions, effects, env->get_symbols()));

                    preconditions.clear();
                    effects.clear();
                    parser = ACTION_DEFINITION;
                }
                else
                {
                    cout << "Effects not specified correctly." << endl;
                    throw;
                }
            }
        }
        input_file.close();
    }

    else
        cout << "Unable to open file";

    return env;
}




class symbo_planner
{
    private:
        class symbo_node
        {
            protected:
                string prev_action;
                list<string> prev_action_inputs;
                vector<symbo_node*> children;
                symbo_node* parent;
                unordered_set<Condition, ConditionHasher, ConditionComparator> state;
                int cost = 0;
                int h = 0; 
                int f = 0; 
            
            public: 
                symbo_node()
                {
                    //do nothing
                }
                symbo_node(string prev_action_in,
                    list<string> prev_action_inputs_in, 
                    symbo_node* parent_in, 
                    unordered_set<Condition, ConditionHasher, ConditionComparator> state_in)
                    {
                        this->prev_action = prev_action_in;
                        this->prev_action_inputs = prev_action_inputs_in;
                        this->state = state_in;
                        this->parent = parent_in;
                    }
                
                void update_f()
                {
                    this->f = cost + h; 
                }
                
                void set_cost(int in_cost)
                {
                    this->cost = in_cost; 
                    update_f();
                }

                void set_h(int in_h)
                {
                    this->h = in_h; 
                    update_f();
                }

                void add_child(symbo_node* child_in)
                {
                    children.push_back(child_in);
                }

                unordered_set<Condition, ConditionHasher, ConditionComparator> get_state()
                {
                    return state;
                }

                string get_prev_action()
                {
                    return prev_action;
                }

                list<string> get_prev_inputs()
                {
                    return prev_action_inputs;
                }

                vector<symbo_node*> get_children()
                {
                    return children;
                }

                symbo_node* get_parent()
                {
                    return parent;
                }

                int get_h()
                {
                    return h;
                }

                int get_f()
                {
                    return f; 
                }

                int get_g()
                {
                    return cost;
                }

                int get_cost()
                {
                    return get_g();
                }
        };

        struct compareFvals
        {
            bool operator()(symbo_node* const& left, symbo_node* const& right) const
            {
                if(left->get_f() != right->get_f())
                {
                    return left->get_f() < right->get_f();
                }
                else
                {
                    //tie-breaking if f is the same
                    return left->get_h() > right->get_h();
                }
            }     
        };
        // sets which automatically sorts 
        set<symbo_node*, compareFvals> open_list;
        unordered_set<symbo_node*> tree;
        unordered_set<Condition, ConditionHasher, ConditionComparator> start_condition;
        unordered_set<Condition, ConditionHasher, ConditionComparator> goal_condition; 
        vector<string> symbols; //vector instead of unordered set for ease of indexing in generating combinations
        unordered_set<Action, ActionHasher, ActionComparator> actions;

        symbo_node* get_next_from_open()
        {
            //pointer dissociates from iterator
            symbo_node* node = *open_list.begin();
            
            //erases value extracted from open list
            open_list.erase(open_list.begin());

            return node;
        }

        void add_to_open(symbo_node* node)
        {
            open_list.insert(node);
        }  

        //returns the h value of the input node
        int calculate_h(symbo_node* node)
        {
            return 0;
        } 

        vector<string> uset_to_vec(unordered_set<string> in_list)
        {
            vector<string> ret_vec; 
            for(auto s : in_list)
            {
                ret_vec.push_back(s);
            }
            return ret_vec;
        }

        void print_set(const list<string>& v) 
        {
            static int count = 0;
            count++;
            cout << "combination number " << count << ": [ ";
            for (auto sym : v) 
            { 
                cout << sym << " ";
            }
            cout << "] " << endl;
        }

        //generates (all permutations)  of k symbols
        void k_combos(const vector<string> symbols, vector<list<string>> &combos, list<string> temp, int offset, int k)
        {
            if (k == 0) {
                // temp.sort(); //moved to sort the symbols on import
                do //once a unique combination is generated, also add the permutations of that set
                {
                    combos.push_back(temp);
                }while(next_permutation(temp.begin(), temp.end()));
                // combos.push_back(temp);
                return;
            }
            for (int i = offset; i <= symbols.size() - k; ++i) 
            {
                temp.push_back(symbols[i]);
                k_combos(symbols, combos, temp, i+1, k-1);
                temp.pop_back();
            }
        }



    public: 
        symbo_planner(unordered_set<Condition, ConditionHasher, ConditionComparator> start, 
            unordered_set<Condition, ConditionHasher, ConditionComparator> goal, 
            unordered_set<string> sym, 
            unordered_set<Action, ActionHasher, ActionComparator> actions_in)
            {
                this->goal_condition = goal;
                this->start_condition = start; 
                this->symbols = uset_to_vec(sym);
                sort(symbols.begin(),symbols.end() ); //sorts symbols lexicographically so that permutations can take place.
                this->actions = actions_in;
            }

        // adapted from https://stackoverflow.com/questions/12991758/creating-all-possible-k-combinations-of-n-items-in-c
        vector<list<string>> generate_sym_combos(int num_symbols)
        {
            vector<list<string>> combinations;
            list<string> tempo;
            k_combos(symbols, combinations, tempo, 0, num_symbols);

            if(false)
            {
                printf("Complete.\n");
                for(int i = 0; i < combinations.size(); ++i)
                {
                    print_set(combinations[i]);
                }
            }

            return combinations;
        }

        void get_neighbors(unordered_set<Condition, ConditionHasher, ConditionComparator> start_cond)
        {
            printf("Generating possible actions for start condition...\n");
            for(auto act : actions) //iterating over all actions
            {
                vector<list<string>> ac_inputs = generate_sym_combos(act.get_num_args());
                cout << "\nevaluating action " << act.toString() << ", " << act.get_num_args() << " inputs , "<< ac_inputs.size() << " generated combos " << endl;
                for(auto input: ac_inputs)
                {
                    // cout << "\t input ( ";
                    // for(auto l : input)
                    // {
                    //     cout << l << " "; 
                    // }
                    // printf(")\n");

                    if(act.preconditions_satisfied(start_cond,input))
                    {
                        // cout<< act.toString();
                        cout << "\t input ( ";
                        for(auto l : input)
                        {
                            cout << l << " "; 
                        }
                        printf(")\n");
                        printf("\t   Condition satisfied!\n");
                        
                    }
                    else
                    {
                        // printf("\t   NOT satisfied!\n");
                    }
                }
            }
        }
};

list<GroundedAction> planner(Env* env)
{
    // this is where you insert your planner
    symbo_planner symbolic_planner = symbo_planner(env->get_initial_ungrounded(), env->get_goal_ungrounded(), env->get_symbols(), env->get_actions());

    printf("\n\n**** Debugging Area **** \n\n");
    // printf("\nChecking the preconditions. Should be satisfied. \n");
    Action test_action = env->get_action("MoveToTable");

    // symbolic_planner.generate_sym_combos(test_action.get_num_args());

    symbolic_planner.get_neighbors(env->get_initial_ungrounded());

    // printf("========\n");
    // printf("Conditions before action: \n");
    // for(auto iter:start_ungrounded)
    // {
    //     cout<< "\t" << iter.toString() << endl;
    // }
    // printf("\n");

    // std::cout << "executing action: " << test_action.toString()  << " with arguments {A,B}" << endl;

    // unordered_set<Condition, ConditionHasher, ConditionComparator> output_conditions = test_action.execute_action(env->get_initial_ungrounded(),{ "A", "B" });
    // printf("Conditions after action: \n"); 
    // for(auto iter:output_conditions)
    // {
    //     cout<< "\t" << iter.toString() << endl;
    // }
    
    printf("\n**** End of Debugging Area **** \n\n\n");
    // blocks world example
    list<GroundedAction> actions;
    actions.push_back(GroundedAction("MoveToTable", { "A", "B" }));
    actions.push_back(GroundedAction("Move", { "C", "Table", "A" }));
    actions.push_back(GroundedAction("Move", { "B", "Table", "C" }));

    return actions;
}

int main(int argc, char* argv[])
{
    // DO NOT CHANGE THIS FUNCTION
    char* filename = (char*)("example.txt");
    if (argc > 1)
        filename = argv[1];

    cout << "Environment: " << filename << endl << endl;
    Env* env = create_env(filename);
    if (print_status)
    {
        cout << *env;
    }

    list<GroundedAction> actions = planner(env);

    cout << "\nPlan: " << endl;
    for (GroundedAction gac : actions)
    {
        cout << gac << endl;
    }

    return 0;
}