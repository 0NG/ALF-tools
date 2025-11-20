#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <array>
#include <tuple>
#include <set>

#include <cxxopts.hpp>

#include "aes_model.hpp"
#include "ortools_kit.hpp"
#include "config.hpp"

using std::cout;
using std::endl;

auto gen_patten(const sat::CpSolverResponse &response, const std::vector<BoolVec> &states, const std::vector<int> &permutation) {
    std::set<int> activePos;
    for (int i = 0; i < states.size() - 1; ++i) {
        for (int j = 0; j < permutation.size(); ++j) {
            bool isActive = 0;
            for (int k = 0; k < 8; ++k)
                isActive |= SolutionIntegerValue(response, states.at(i).at(8 * j + k));

            if (isActive) activePos.insert(j);
        }
    }

    std::vector<int> pattern;
    for (const auto &pos : permutation)
        if (activePos.contains(pos))
            pattern.push_back(pos);
        else
            pattern.push_back(-1);

    return pattern;
}

auto permuate(sat::CpModelBuilder &model, BoolVec &state, const std::vector<int> &permutation) {
    const auto tSize = (state.size() <= 128) * (state.size() % 8) + (state.size() > 128) * (state.size() - 128);
    BoolVec output;

    for (const auto &i : permutation) {
        if (i == -1) break;
        for (int j = 0; j < 8; ++j)
            output.push_back(state.at(8 * i + j));
    }
    for (int j = 0; j < tSize; ++j)
        output.push_back(state.at(state.size() - tSize + j));
    
    return output;
}

auto subbytes(sat::CpModelBuilder &model, BoolVec &state, std::vector<BoolVar> &actives) {
    const auto tSize = (state.size() <= 128) * (state.size() % 8) + (state.size() > 128) * (state.size() - 128);
    const auto nSboxes = (state.size() - tSize) / 8;
    BoolVec output;

    for (int i = 0; i < nSboxes; ++i) {
        for (int j = 0; j < 8; ++j) output.push_back(model.NewBoolVar());

        BoolVec tmpIn;
        BoolVec tmpOut;
        for (int j = 0; j < 8; ++j) tmpIn.push_back(state.at(8 * i + j));
        for (int j = 0; j < 8; ++j) tmpOut.push_back(output.at(8 * i + j));

        BoolVec tmpInNot;
        BoolVec tmpOutNot;
        for (int j = 0; j < 8; ++j) tmpInNot.push_back(state.at(8 * i + j).Not());
        for (int j = 0; j < 8; ++j) tmpOutNot.push_back(output.at(8 * i + j).Not());

        auto isActive =  model.NewBoolVar();
        model.AddBoolOr(tmpIn).OnlyEnforceIf(isActive);
        model.AddBoolAnd(tmpInNot).OnlyEnforceIf(isActive.Not());
        model.AddBoolOr(tmpOut).OnlyEnforceIf(isActive);
        model.AddBoolAnd(tmpOutNot).OnlyEnforceIf(isActive.Not());

        actives.push_back(isActive);
    }

    for (int j = 0; j < tSize; ++j)
        output.push_back(state.at(state.size() - tSize + j));
    
    return output;
}

auto padmc(sat::CpModelBuilder &model, BoolVec &state) {
    const auto tSize = (state.size() <= 128) * (state.size() % 8) + (state.size() > 128) * (state.size() - 128);
    const auto nSize = (state.size() - tSize) / 8;
    const auto nCol = (state.size() - tSize + 31) / 32;

    auto padHelper = [&](const int caseSize){
        BoolVec output;
        const auto alphaShuffle = CipherConfig::profiles.at(caseSize).at(0);
        for (const auto p : alphaShuffle) {
            if (p == -1) {
                for (int i = 0; i < 8; ++i)
                    output.push_back(model.FalseVar());
            } else {
                for (int i = 0; i < 8; ++i)
                    output.push_back(state.at(8 * p + i));
            }
        }
        return output;
    };

    auto output = padHelper(nSize);

    for (int i = 0; i < 128 - 32 * nCol; ++i) output.push_back(model.FalseVar());

    auto result = NewBoolVec(model, 128);
    bmXbv(model, MCbm, output, result);

    for (int i = 0; i < 128 - 32 * nCol; ++i) result.pop_back();

    for (int j = 0; j < tSize; ++j)
        result.push_back(state.at(state.size() - tSize + j));
    
    return result;
}

auto exchange(sat::CpModelBuilder &model, BoolVec &state, const int nSize) {
    const auto tSize = (state.size() <= 128) * (state.size() % 8) + (state.size() > 128) * (state.size() - 128);
    const auto nCol = (state.size() - tSize + 31) / 32;

    auto exchangeHelper = [&](){
        BoolVec output;
        auto pt = NewBoolVec(model, tSize);
        for (int i = 8 - tSize; i < 8; ++i) {
            model.AddBoolXor({
                pt.at(i - (8 - tSize)),
                state.at( 0 + i),
                state.at( 8 + i),
                state.at(16 + i),
                state.at(24 + i),
                model.TrueVar()
            });
        }

        // state ^ betaShuffle(state) ^ etShuffle(et)
        const auto betaShuffle = CipherConfig::profiles.at(nSize).at(1);
        const auto etShuffle = CipherConfig::profiles.at(nSize).at(2);
        for (int i = 0; i < nSize; ++i) {
            for (int j = 0; j < 8 - tSize * etShuffle.at(i); ++j) {
                if (betaShuffle.at(i) == -1) {
                    output.push_back(state.at(8 * i + j));
                } else {
                    auto tmp = diff_var_xor(
                        model,
                        state.at(8 * i + j),
                        state.at(8 * betaShuffle.at(i) + j)
                    );
                    output.push_back(tmp);
                }
            }
            for (int j = 0; j < tSize * etShuffle.at(i); ++j) {
                if (betaShuffle.at(i) == -1) {
                    auto tmp = diff_var_xor(
                        model,
                        state.at(8 * i + (8 - tSize) + j),
                        state.at(32 * nCol + j)
                    );
                    output.push_back(tmp);
                } else {
                    auto tmp = diff_var_xor(
                        model,
                        state.at(8 * i + (8 - tSize) + j),
                        state.at(8 * betaShuffle.at(i) + (8 - tSize) + j),
                        state.at(32 * nCol + j)
                    );
                    output.push_back(tmp);
                }
            }
        }

        for (int i = 0; i < tSize; ++i) {
            auto tmp = model.NewBoolVar();
            model.AddBoolXor({tmp, pt.at(i), state.at(32 * nCol + i), model.TrueVar()});
            output.push_back(tmp);
        }
        return output;
    };

    if (nSize == 1) { // case 1
        BoolVec output;
        for (int i = 0; i < 8 - tSize - 1; ++i) output.push_back(state.at(i));
        for (int i = 8 - tSize - 1; i < 7; ++i) {
            auto tmp = model.NewBoolVar();
            model.AddBoolXor({state.at(i), state.at(8 + i - (8 - tSize - 1)), tmp, model.TrueVar()});
            output.push_back(tmp);
        }
        output.push_back(state.at(7));

        for (int i = 0; i < tSize; ++i) {
            auto tmp = model.NewBoolVar();
            model.AddBoolXor({state.at((8 - tSize) + i), state.at(8 + i), tmp, model.TrueVar()});
            output.push_back(tmp);
        }

        return output;

    } else if (nSize == 16) {
        const auto tSize0 = (tSize <= 8) * tSize + (tSize > 8) * 8;
        const auto tSize1 = tSize - tSize0;
        auto pt0 = NewBoolVec(model, tSize0);
        for (int i = 8 - tSize0; i < 8; ++i) {
            model.AddBoolXor({
                pt0.at(i - (8 - tSize0)),
                state.at( 0 + i),
                state.at( 8 + i),
                state.at(16 + i),
                state.at(24 + i),
                model.TrueVar()
            });
        }
        auto pt1 = NewBoolVec(model, tSize1);
        for (int i = 8 - tSize1; i < 8; ++i) {
            model.AddBoolXor({
                pt1.at(i - (8 - tSize1)),
                state.at(32 + i),
                state.at(40 + i),
                state.at(48 + i),
                state.at(56 + i),
                model.TrueVar()
            });
        }

        BoolVec output;
        // state ^ betaShuffle(state) ^ etShuffle(et)
        for (int i = 0; i < nSize; ++i) {
            if (i < 4) {
                for (int j = 0; j < 8 - tSize0; ++j) {
                    output.push_back(state.at(8 * i + j));
                }
                for (int j = 0; j < tSize0; ++j) {
                    auto tmp = diff_var_xor(
                        model,
                        state.at(8 * i + (8 - tSize0) + j),
                        state.at(32 * nCol + j)
                    );
                    output.push_back(tmp);
                }
            } else if (i < 8) {
                for (int j = 0; j < 8 - tSize1; ++j) {
                    output.push_back(state.at(8 * i + j));
                }
                for (int j = 0; j < tSize1; ++j) {
                    auto tmp = diff_var_xor(
                        model,
                        state.at(8 * i + (8 - tSize1) + j),
                        state.at(32 * nCol + 8 + j)
                    );
                    output.push_back(tmp);
                }
            } else {
                for (int j = 0; j < 8; ++j)
                    output.push_back(state.at(8 * i + j));
            }
        }

        for (int i = 0; i < tSize0; ++i) {
            auto tmp = model.NewBoolVar();
            model.AddBoolXor({tmp, pt0.at(i), state.at(32 * nCol + i), model.TrueVar()});
            output.push_back(tmp);
        }
        for (int i = 0; i < tSize1; ++i) {
            auto tmp = model.NewBoolVar();
            model.AddBoolXor({tmp, pt1.at(i), state.at(32 * nCol + 8 + i), model.TrueVar()});
            output.push_back(tmp);
        }
        return output;
    }

    return exchangeHelper();
}

auto minDiffSboxalfnt(const int nSize, const int tSize, const int nRound, const int step, const std::vector<int> &permutation, const int nThread, const int logLevel, const int timeLimit = -1, const int threshold = -1) -> std::tuple<int, int, int> {
    SatParameters parameters;
    parameters.set_num_search_workers(nThread);
    parameters.set_log_search_progress(logLevel == 1);
    if (timeLimit != -1) parameters.set_max_time_in_seconds(timeLimit);

    CpModelBuilder cpModel;
    std::vector<BoolVec> states;
    std::vector<BoolVec> intermediates;
    std::vector<BoolVar> actives;

    auto inputDiff = NewBoolVec(cpModel, 8 * nSize + tSize);
    states.push_back(inputDiff);

    for (int r = 0; r < nRound; ++r) {
        auto lastState = states.at(r);
        lastState = permuate(cpModel, lastState, permutation);
        intermediates.push_back(lastState);
        lastState = subbytes(cpModel, lastState, actives);
        intermediates.push_back(lastState);
        if (nSize != 1) lastState = padmc(cpModel, lastState);
        intermediates.push_back(lastState);

        lastState = exchange(cpModel, lastState, nSize);
        //intermediates.push_back(lastState);

        states.push_back(lastState);

    }

    cpModel.AddGreaterOrEqual(LinearExpr::Sum(inputDiff), 1);

    auto obj = cpModel.NewIntVar(Domain(0, actives.size()));
    cpModel.AddEquality(obj, LinearExpr::Sum(actives));
    //cpModel.AddGreaterOrEqual(obj, 18);
    cpModel.Minimize(obj);

    /*===========================================================================*/
    Model model;
    std::atomic<bool> stopped(false);
    model.GetOrCreate<TimeLimit>()->RegisterExternalBooleanAsLimit(&stopped);
    model.Add(NewFeasibleSolutionObserver([&](const CpSolverResponse& r) {
        const auto newObj = SolutionIntegerValue(r, obj);

        if (threshold != -1 && newObj < threshold) stopped = true;
    }));
    //parameters.add_subsolvers("core");
    //parameters.add_subsolvers("default_lp");
    //parameters.add_subsolvers("rins_lp_lns");
    model.Add(NewSatParameters(parameters));
    /*===========================================================================*/
    
    auto builtModel = cpModel.Build();
    //cout << model_built.DebugString() << endl;

    //const auto response = Solve(builtModel);
    //const auto response = SolveWithParameters(builtModel, parameters);
    const auto response = SolveCpModel(builtModel, &model);
    const auto status = response.status();

    // CpSolverStatus::INFEASIBLE = 3
    if (status == CpSolverStatus::OPTIMAL || status == CpSolverStatus::FEASIBLE) {
        auto result = SolutionIntegerValue(response, obj);
        const int upperBound = std::max(response.objective_value(), response.best_objective_bound());
        const int lowerBound = response.inner_objective_lower_bound();
        if (upperBound < lowerBound) throw std::logic_error("something wrong with getting the range");

        if (logLevel == 0) return {result, lowerBound, upperBound};

        //if (logLevel == 2) return {result, lowerBound, upperBound};

        cout << "========== states ==========" << endl;
        for (const auto &s : states) {
            cout << s.size() << endl;
            print_solution_bytes(response, s); cout << endl;
        }

        //const auto pattern = gen_patten(response, states, permutation);
        //for (const auto &p : pattern)
        //    cout << p << ", ";
        //cout << endl;

        cout << "========== intermediates ==========" << endl;
        for (const auto &s : intermediates) {
            print_solution_bytes(response, s); cout << endl;
        }

        cout << "====================" << endl;
        cout << "obj  : " << result << endl;
        cout << "wall time: " << response.wall_time() << endl;

        return {result, lowerBound, upperBound};
    }

    if (logLevel == 1) {
        cout << "====================" << endl;
        cout << "infeasible: " << endl;
    }
    return {-1, -1, -1};
}

auto main(int argc, char *argv[]) -> int {
    if (argc > 1) {

        cxxopts::Options options("./executable");
        options.add_options()
            ("sigma", "extra permutation at the beginning (0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15)", cxxopts::value<std::vector<int>>())
            ("alpha", "", cxxopts::value<std::vector<int>>()->default_value("-2"))
            ("beta", "", cxxopts::value<std::vector<int>>()->default_value("-2"))
            ("et", "", cxxopts::value<std::vector<int>>()->default_value("-2"))

            ("nsize", "size of AES (in byte) [1, 2, 3, ..., 16]", cxxopts::value<int>())
            ("tsize", "size of AES (in byte) [1, 2, 3, ..., 16]", cxxopts::value<int>())
            ("n,round", "Number of rounds (clocks) [4, 5, 6, 7, ...]", cxxopts::value<int>())
            ("step", "", cxxopts::value<int>()->default_value("1"))
            ("info", "Print parameters or not [0, 1]", cxxopts::value<int>()->default_value("0"))
            ("r,restart", "Number of restarts", cxxopts::value<int>()->default_value("1"))
            ("j,thread", "Number of threads", cxxopts::value<int>()->default_value("8"))
            ("b,threshold", "Stop when a solution less than this value is found. (default: no threshold)", cxxopts::value<int>()->default_value("-1"))
            ("l,log", "Log level: 0=only results, 1=detailed search progress, 2=only patterns (default: 0) [0, 1]", cxxopts::value<int>()->default_value("0"))
            ("t,time", "Timeout, time limit (in seconds, default: unlimited)", cxxopts::value<int>()->default_value("-1"))
            ("h,help", "Help menu", cxxopts::value<bool>()->default_value("false"));

        auto results = options.parse(argc, argv);
        if (results["help"].as<bool>()) {
            cout << options.help() << endl;
            return 0;
        }

        const auto extraPerm = results["sigma"].as<std::vector<int>>();
        const auto alpha = results["alpha"].as<std::vector<int>>();
        const auto beta = results["beta"].as<std::vector<int>>();
        const auto et = results["et"].as<std::vector<int>>();

        const auto nSize = results["nsize"].as<int>();
        const auto tSize = results["tsize"].as<int>();
        const auto step = results["step"].as<int>();
        const auto nRestart = results["restart"].as<int>();
        const auto nRound = results["round"].as<int>();
        const auto nThread = results["thread"].as<int>();
        const auto threshold = results["threshold"].as<int>();
        const auto logLevel = results["log"].as<int>();
        const auto timeLimit = results["time"].as<int>();
        const auto info = results["info"].as<int>();

        if (alpha.at(0) != -2) CipherConfig::profiles.at(nSize).at(0) = alpha;
        if ( beta.at(0) != -2) CipherConfig::profiles.at(nSize).at(1) = beta;
        if (   et.at(0) != -2) CipherConfig::profiles.at(nSize).at(2) = et;
        for (int i = 0; i < 16; ++i) {
            const auto tmp = CipherConfig::profiles.at(nSize).at(2).at(i);
            CipherConfig::profiles.at(nSize).at(2).at(i) = (tmp != -1);
        }

        if (info) {
            cout << "rounds: " << nRound << endl;
            cout << "number of threads: " << nThread << endl;
            cout << "threshold: " << threshold << endl;
            cout << "log level: " << logLevel << endl;
            cout << "time out in " << timeLimit << " seconds." << endl;
            cout << "restart: " << nRestart << " times." << endl;
        }

        auto minActives = 0x0fffffff;
        auto lowerBound = -1;
        auto upperBound = 0x0fffffff;

        for (int i = 0; i < nRestart; ++i) {
            const auto [m, l, u] = minDiffSboxalfnt(nSize, tSize, nRound, step, extraPerm, nThread, logLevel, timeLimit, threshold);
            minActives = (m != -1 && m < minActives) ? m : minActives;
            lowerBound = (l != -1 && l > lowerBound) ? l : lowerBound;
            upperBound = (u != -1 && u < upperBound) ? u : upperBound;
        }

        if (lowerBound == upperBound && minActives != lowerBound)
            throw std::logic_error("something wrong with getting the range");

        cout << "minimum number of active sboxes: " << upperBound << endl;

        return upperBound;
    }

    return 0;
}
