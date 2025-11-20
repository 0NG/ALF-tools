#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <array>
#include <tuple>
#include <numeric>

#include <cxxopts.hpp>

#include "aes_model.hpp"
#include "ortools_kit.hpp"
#include "config.hpp"

using std::cout;
using std::endl;

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

    auto padHelper = [&](){
        const auto alphaShuffle = CipherConfig::profiles.at(nSize).at(0);
        const auto shuffleM = CipherConfig::pos2m(alphaShuffle, nSize);
        const auto shuffleBm = compute_general_bm(shuffleM);

        std::vector<BoolVar> seg;
        for (int i = 0; i < 8 * nSize; ++i) seg.push_back(state.at(i));

        return linear_bmXbv_consumed(model, shuffleBm, seg, true);
    };

    auto output = padHelper();

    for (int i = 0; i < 128 - 32 * nCol; ++i) output.push_back(model.FalseVar());

    auto result = NewBoolVec(model, 128);
    std::vector<BoolVec> tmpVars;
    tmpVars.clear();

    for (int row = 0; row < 128; ++row) {
        tmpVars.push_back({});

        for (int col = 0; col < 128; ++col) {
            auto tmp = model.NewBoolVar();
            tmpVars.at(row).push_back(tmp);

            if (MCbm.at(row).at(col))
                model.AddEquality(result.at(row), tmp);
        }
    }
    for (int col = 0; col < 128; ++col) {
        std::vector<BoolVar> tmpSumVars{{model.TrueVar(), output.at(col)}};

        for (int row = 0; row < 128; ++row)
            if (MCbm.at(row).at(col))
                tmpSumVars.push_back(tmpVars.at(row).at(col));

        model.AddBoolXor(tmpSumVars);
    }

    for (int i = 0; i < 128 - 32 * nCol; ++i)
        model.AddEquality(result.at(32 * nCol + i), model.FalseVar());
    for (int i = 0; i < 128 - 32 * nCol; ++i)
        result.pop_back();

    for (int j = 0; j < tSize; ++j)
        result.push_back(state.at(state.size() - tSize + j));

    return result;
}

auto exchange(sat::CpModelBuilder &model, BoolVec &state, const int nSize) {
    const auto tSize = (state.size() <= 128) * (state.size() % 8) + (state.size() > 128) * (state.size() - 128);
    const auto nCol = (state.size() - tSize + 31) / 32;

    auto exchangeHelper = [&](){
        BoolVec output;
        BoolVec alignedPart;
        for (int i = 0; i < state.size() - tSize; ++i) alignedPart.push_back(state.at(i));
        auto copies = linear_bv_copy(model, alignedPart, 2);

        const auto betaBm = compute_general_bm(
            CipherConfig::pos2m(
                CipherConfig::profiles.at(nSize).at(1),
                4 * nCol
            )
        );
        auto [afterBeta, copySeg] = linear_bmXbv_reserved(model, betaBm, copies.at(0));

        BoolVec beforeEt;
        for (int i = 0; i < 4 * nCol; ++i) {
            if (CipherConfig::profiles.at(nSize).at(1).at(i) == -1) {
                for (int j = 0; j < 8; ++j)
                    beforeEt.push_back(copySeg.at(8 * i + j));
            } else {
                for (int j = 0; j < 8; ++j) {
                    auto tmp = model.NewBoolVar();
                    model.AddEquality(tmp, copySeg.at(8 * i + j));
                    model.AddEquality(tmp, afterBeta.at(8 * i + j));
                    beforeEt.push_back(tmp);
                }
            }
        }

        for (int i = 0; i < 8 * (4 * nCol - nSize); ++i) model.AddEquality(beforeEt.at(beforeEt.size() - 1 - i), model.FalseVar());
        for (int i = 0; i < 8 * (4 * nCol - nSize); ++i) beforeEt.pop_back();

        std::vector<BoolVar> seg1;
        for (int i = 0; i < tSize; ++i) seg1.push_back(state.at(32 * nCol + i));

        const auto etShuffle = CipherConfig::profiles.at(nSize).at(2);
        auto copyEt = linear_bv_copy(model, seg1, std::accumulate(etShuffle.begin(), etShuffle.end(), 1));

        int copyEtCnt = 1;
        for (int i = 0; i < nSize; ++i) {
            if (etShuffle.at(i)) {
                for (int j = 0; j < 8 - tSize; ++j) output.push_back(beforeEt.at(8 * i + j));

                for (int j = 0; j < tSize; ++j) {
                    auto tmp = model.NewBoolVar();
                    model.AddEquality(tmp, beforeEt.at(8 * i + (8 - tSize) + j));
                    model.AddEquality(tmp, copyEt.at(copyEtCnt).at(j));
                    output.push_back(tmp);
                }

                ++copyEtCnt;
            } else {
                for (int j = 0; j < 8; ++j)
                    output.push_back(beforeEt.at(8 * i + j));
            }
        }

        std::vector<std::vector<int>> parityM;
        {
            parityM.push_back({});
            for (int j = 0; j < 4; ++j) parityM.at(0).push_back(1);
            for (int j = 0; j < copies.at(1).size() / 8 - 4; ++j) parityM.at(0).push_back(0);
        }
        for (int i = 0; i < copies.at(1).size() / 8 - 1; ++i) {
            parityM.push_back({});
            for (int j = 0; j < copies.at(1).size() / 8; ++j) parityM.at(i + 1).push_back(0);
        }
        const auto parityBm = compute_general_bm(parityM);
        auto parity = linear_bmXbv_consumed(model, parityBm, copies.at(1));

        for (int j = 0; j < 8 - tSize; ++j)
            model.AddEquality(parity.at(j), model.FalseVar());
        for (int j = 0; j < tSize; ++j) {
            auto tmp = model.NewBoolVar();
            model.AddEquality(tmp, parity.at((8 - tSize) + j));
            model.AddEquality(tmp, copyEt.at(0).at(j));
            output.push_back(tmp);
        }

        return output;
    };

    if (nSize == 1) { // case 1
        BoolVec stateCopy0;
        BoolVec stateCopy1;
        for (int i = 0; i < 8 - tSize; ++i) stateCopy0.push_back(state.at(i));
        for (int i = 0; i < tSize; ++i) {
            auto tmpCopies = linear_var_copy(model, state.at((8 - tSize) + i), 2);
            stateCopy0.push_back(tmpCopies.at(0));
            stateCopy1.push_back(tmpCopies.at(1));
        }

        BoolVec etCopy0;
        BoolVec etCopy1;
        for (int i = 0; i < tSize; ++i) {
            auto tmpCopies = linear_var_copy(model, state.at(8 + i), 2);
            etCopy0.push_back(tmpCopies.at(0));
            etCopy1.push_back(tmpCopies.at(1));
        }

        BoolVec output;
        for (int i = 0; i < 8 - tSize - 1; ++i) output.push_back(stateCopy0.at(i));
        for (int i = 8 - tSize - 1; i < 7; ++i) {
            auto tmp = linear_var_xor(model, stateCopy0.at(i), etCopy1.at(i));
            output.push_back(tmp);
        }
        output.push_back(stateCopy0.at(7));

        for (int i = 0; i < tSize; ++i) {
            auto tmp = linear_var_xor(model, stateCopy1.at(i), etCopy0.at(i));
            output.push_back(tmp);
        }

        return output;
    } else if (nSize == 16) {
        BoolVec output;
        BoolVec alignedPart;
        for (int i = 0; i < state.size() - tSize; ++i) alignedPart.push_back(state.at(i));
        auto copies = linear_bv_copy(model, alignedPart, 2);

        const auto tSize0 = (tSize <= 8) * tSize + (tSize > 8) * 8;
        const auto tSize1 = tSize - tSize0;

        std::vector<BoolVar> seg0;
        for (int i = 0; i < tSize0; ++i) seg0.push_back(state.at(32 * nCol + i));
        auto copyEt0 = linear_bv_copy(model, seg0, 5);

        std::vector<BoolVar> seg1;
        for (int i = 0; i < tSize1; ++i) seg1.push_back(state.at(32 * nCol + 8 + i));
        auto copyEt1 = linear_bv_copy(model, seg1, 5);

        for (int i = 0; i < nSize; ++i) {
            if (i < 4) {
                for (int j = 0; j < 8 - tSize0; ++j) output.push_back(copies.at(0).at(8 * i + j));

                for (int j = 0; j < tSize0; ++j) {
                    auto tmp = model.NewBoolVar();
                    model.AddEquality(tmp, copies.at(0).at(8 * i + (8 - tSize0) + j));
                    model.AddEquality(tmp, copyEt0.at(i + 1).at(j));
                    output.push_back(tmp);
                }
            } else if (i < 8) {
                for (int j = 0; j < 8 - tSize1; ++j) output.push_back(copies.at(0).at(8 * i + j));

                for (int j = 0; j < tSize1; ++j) {
                    auto tmp = model.NewBoolVar();
                    model.AddEquality(tmp, copies.at(0).at(8 * i + (8 - tSize1) + j));
                    model.AddEquality(tmp, copyEt1.at(i - 4 + 1).at(j));
                    output.push_back(tmp);
                }
            } else {
                for (int j = 0; j < 8; ++j)
                    output.push_back(copies.at(0).at(8 * i + j));
            }
        }

        std::vector<std::vector<int>> parityM;
        {
            parityM.push_back({});
            for (int j = 0; j < 4; ++j) parityM.at(0).push_back(1);
            for (int j = 0; j < copies.at(1).size() / 8 - 4; ++j) parityM.at(0).push_back(0);

            parityM.push_back({});
            for (int j = 0; j < 4; ++j) parityM.at(1).push_back(0);
            for (int j = 0; j < 4; ++j) parityM.at(1).push_back(1);
            for (int j = 0; j < copies.at(1).size() / 8 - 8; ++j) parityM.at(1).push_back(0);
        }
        for (int i = 0; i < copies.at(1).size() / 8 - 2; ++i) {
            parityM.push_back({});
            for (int j = 0; j < copies.at(1).size() / 8; ++j) parityM.at(i + 2).push_back(0);
        }
        const auto parityBm = compute_general_bm(parityM);
        auto parity = linear_bmXbv_consumed(model, parityBm, copies.at(1));

        for (int j = 0; j < 8 - tSize0; ++j)
            model.AddEquality(parity.at(j), model.FalseVar());
        for (int j = 0; j < 8 - tSize1; ++j)
            model.AddEquality(parity.at(8 + j), model.FalseVar());

        for (int j = 0; j < tSize0; ++j) {
            auto tmp = model.NewBoolVar();
            model.AddEquality(tmp, parity.at((8 - tSize0) + j));
            model.AddEquality(tmp, copyEt0.at(0).at(j));
            output.push_back(tmp);
        }
        for (int j = 0; j < tSize1; ++j) {
            auto tmp = model.NewBoolVar();
            model.AddEquality(tmp, parity.at(8 + (8 - tSize1) + j));
            model.AddEquality(tmp, copyEt1.at(0).at(j));
            output.push_back(tmp);
        }

        return output;
    }

    return exchangeHelper();
}

auto minLinearSboxalfnt(const int nSize, const int tSize, const int nRound, const std::vector<int> &permutation, const int nThread, const int logLevel, const int timeLimit = -1, const int threshold = -1) -> std::tuple<int, int, int> {
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
    //parameters.add_subsolvers("no_lp");
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
            print_solution_bytes(response, s); cout << endl;
        }

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
            ("nsize", "size of AES (in byte) [1, 2, 3, ..., 16]", cxxopts::value<int>())
            ("tsize", "size of AES (in byte) [1, 2, 3, ..., 16]", cxxopts::value<int>())
            ("n,round", "Number of rounds (clocks) [4, 5, 6, 7, ...]", cxxopts::value<int>())
            ("alpha", "", cxxopts::value<std::vector<int>>()->default_value("-2"))
            ("beta", "", cxxopts::value<std::vector<int>>()->default_value("-2"))
            ("et", "", cxxopts::value<std::vector<int>>()->default_value("-2"))

            ("j,thread", "Number of threads", cxxopts::value<int>()->default_value("8"))
            ("b,threshold", "Stop when a solution less than this value is found. (default: no threshold)", cxxopts::value<int>()->default_value("-1"))

            ("r,restart", "Number of restarts", cxxopts::value<int>()->default_value("1"))
            ("l,log", "Log level: 0=only results, 1=detailed search progress, 2=only patterns (default: 0) [0, 1]", cxxopts::value<int>()->default_value("0"))
            ("t,time", "Timeout, time limit (in seconds, default: unlimited)", cxxopts::value<int>()->default_value("-1"))

            ("info", "Print parameters or not [0, 1]", cxxopts::value<int>()->default_value("0"))
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
            const auto [m, l, u] = minLinearSboxalfnt(nSize, tSize, nRound, extraPerm, nThread, logLevel, timeLimit, threshold);
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
