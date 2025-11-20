#ifndef ORTOOLS_KIT_HPP
#define ORTOOLS_KIT_HPP

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cassert>

#include <ortools/sat/cp_model.h>
#include <ortools/util/time_limit.h>

using namespace operations_research;
using namespace operations_research::sat;

using BoolVec = std::vector<sat::BoolVar>;
using IntVec = std::vector<sat::IntVar>;

BoolVec NewBoolVec(sat::CpModelBuilder &model, const int size)
{
    // bv[0] is the lsb
    BoolVec bv;
    for (int i = 0; i < size; ++i) bv.push_back(model.NewBoolVar());
    return bv;
}

IntVec BV2IV(BoolVec &bv)
{
    const int len = bv.size();

    IntVec iv;
    for (int i = 0; i < len; ++i) iv.push_back(sat::IntVar(bv[i]));
    return iv;
}

void BVXor(sat::CpModelBuilder &model, BoolVec &bv0, BoolVec &bv1, BoolVec &bv2)
{
    /*
     * bv0 ^ bv1 = bv2
     */
    const int len = bv0.size();

    for (int i = 0; i < len; ++i) {
        model.AddBoolXor({bv0[i], bv1[i], bv2[i], model.TrueVar()});
    }

    return;
}

void BVAssignIf(sat::CpModelBuilder &model, BoolVec &bv, const std::vector<std::vector<int64_t>> &values, sat::IntVar b)
{
    auto iv = BV2IV(bv);
    iv.push_back(b);
    auto table = model.AddAllowedAssignments(iv);

    for (auto &value : values) {
        table.AddTuple(value);
    }

    return;
} 

void BVAssignIf(sat::CpModelBuilder &model, BoolVec &bv, const std::vector<std::vector<int64_t>> &values, sat::BoolVar b)
{
    BVAssignIf(model, bv, values, sat::IntVar(b));
    return;
} 

void BVAssign(sat::CpModelBuilder &model, BoolVec &bv, const std::vector<std::vector<int64_t>> &values)
{
    auto iv = BV2IV(bv);
    auto table = model.AddAllowedAssignments(iv);

    for (auto &value : values) {
        table.AddTuple(value);
    }

    return;
} 

BoolVec BVRor(const BoolVec &bv, const int rotation)
{
    const int len = bv.size();
    const int rn = rotation % len;

    BoolVec output;
    for (int i = rn; i < len; ++i) {
        output.push_back(bv[i]);
    }
    for (int i = 0; i < rn; ++i) {
        output.push_back(bv[i]);
    }
    return output;
} 

BoolVec BVRol(BoolVec &bv, const int rotation)
{
    const int len = bv.size();
    const int rn = rotation % len;

    return BVRor(bv, len - rn);
} 

[[nodiscard]]
BoolVec BVXor(sat::CpModelBuilder &model, BoolVec &bv0, BoolVec &bv1)
{
    const int len = bv0.size();
    BoolVec bv2 = NewBoolVec(model, len);

    for (int i = 0; i < len; ++i) {
        model.AddBoolXor({bv0[i], bv1[i], bv2[i], model.TrueVar()});
    }

    return bv2;
}

void BVXor3(sat::CpModelBuilder &model, BoolVec &bv0, BoolVec &bv1, BoolVec &bv2, BoolVec &bv3)
{
    const int len = bv0.size();

    for (int i = 0; i < len; ++i) {
        model.AddBoolXor({bv0[i], bv1[i], bv2[i], bv3[i], model.TrueVar()});
    }

    return;
}

[[nodiscard]]
BoolVec bmXbv(sat::CpModelBuilder &model, const bm<128> &m, BoolVec &bv)
{
    BoolVec r = NewBoolVec(model, 128);

    for (int row = 0; row < 128; ++row) {
        BoolVec tmp{{model.TrueVar(), r[row]}};
        for (int col = 0; col < 128; ++col)
            if (m[row][col])
                tmp.push_back(bv[col]);
        model.AddBoolXor(tmp);
    }

    return r;
}

void bmXbv(sat::CpModelBuilder &model, const bm<128> &m, BoolVec &bv, BoolVec &r)
{
    for (int row = 0; row < 128; ++row) {
        BoolVec tmp{{model.TrueVar(), r[row]}};
        for (int col = 0; col < 128; ++col)
            if (m[row][col])
                tmp.push_back(bv[col]);
        model.AddBoolXor(tmp);
    }

    return;
}

void bvXbm(sat::CpModelBuilder &model, BoolVec &bv, const bm<128> &m, BoolVec &r)
{
    for (int col = 0; col < 128; ++col) {
        BoolVec tmp{{model.TrueVar(), r[col]}};
        for (int row = 0; row < 128; ++row)
            if (m[row][col])
                tmp.push_back(bv[row]);
        model.AddBoolXor(tmp);
    }

    return;
}

[[nodiscard]]
BoolVec bvXbm(sat::CpModelBuilder &model, BoolVec &bv, const bm<128> &m)
{
    BoolVec r = NewBoolVec(model, 128);
    bvXbm(model, bv, m, r);
    return r;
}

template<typename... Args>
void noneZeroExp(sat::CpModelBuilder &model, Args&&... args)
{
    BoolVec exp;
    auto helper = [&exp] <typename Arg> (Arg &&arg) {
        for (auto &v : arg) exp.push_back(v);
    };
    (helper(std::forward<Args>(args)), ...);

    model.AddGreaterOrEqual(LinearExpr::Sum(exp), 1);
    return;
}

void print_solution_bytes(const sat::CpSolverResponse &response, const BoolVec &x) {
    int cnt = 0;
    int tmp = 0;

    for (auto &b : x) {
        tmp = (tmp << 1) | SolutionIntegerValue(response, b);
        ++cnt;

        if (cnt == 8) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << tmp << std::dec << ", ";
            cnt = 0;
            tmp = 0;
        }
    }

    if (cnt) std::cout << std::hex << std::setw(2) << std::setfill('0') << tmp << std::dec << ", ";
    return;
}

template<typename... Args>
auto linear_var_xor(sat::CpModelBuilder &model, BoolVar &v0, BoolVar &v1, Args&&... args) -> BoolVar {
    auto result = model.NewBoolVar();

    model.AddEquality(result, v0);
    model.AddEquality(result, v1);

    auto helper = [&model, &result] <typename Arg> (Arg &&arg) {
        model.AddEquality(result, arg);
    };
    (helper(std::forward<Args>(args)), ...);

    return result;
}

template<typename... Args>
auto diff_var_xor(sat::CpModelBuilder &model, BoolVar &v0, BoolVar &v1, Args&&... args) -> BoolVar {
    BoolVec exp;
    exp.push_back(v0);
    exp.push_back(v1);

    auto helper = [&exp] <typename Arg> (Arg &&arg) { exp.push_back(arg); };
    (helper(std::forward<Args>(args)), ...);

    auto result = model.NewBoolVar();
    exp.push_back(result);
    exp.push_back(model.TrueVar());
    model.AddBoolXor(exp);

    return result;
}

auto linear_bv_copy(sat::CpModelBuilder &model, BoolVec &bv, const int nCopy) -> std::vector<BoolVec> {
    std::vector<BoolVec> copies;
    const int len = bv.size();

    for (int i = 0; i < nCopy; ++i) {
        copies.push_back({});

        for (int j = 0; j < len; ++j)
            copies.at(i).push_back(model.NewBoolVar());
    }

    for (int i = 0; i < len; ++i) {
        std::vector<BoolVar> tmp;

        for (int j = 0; j < nCopy; ++j) tmp.push_back(copies.at(j).at(i));
        tmp.push_back(bv.at(i));
        tmp.push_back(model.TrueVar());
        model.AddBoolXor(tmp);
    }

    return copies;
}

auto linear_var_copy(sat::CpModelBuilder &model, BoolVar &v, const int nCopy) -> std::vector<BoolVar> {
    std::vector<BoolVar> copies;

    for (int i = 0; i < nCopy; ++i)
        copies.push_back(model.NewBoolVar());

    copies.push_back(v);
    copies.push_back(model.TrueVar());
    model.AddBoolXor(copies);

    copies.pop_back();
    copies.pop_back();

    return copies;
}

auto linear_bmXbv_consumed(sat::CpModelBuilder &model, const std::vector<std::vector<int>> &bm, BoolVec &bv, const bool isShuffle = false) -> BoolVec {
    const int inLen = bv.size();
    const int outLen = bm.size();
    auto result = NewBoolVec(model, outLen);

    std::vector<BoolVec> tmpVars;
    tmpVars.clear();

    for (int row = 0; row < outLen; ++row) {
        tmpVars.push_back({});

        bool hasOne = 0;
        for (int col = 0; col < inLen; ++col) {
            auto tmp = model.NewBoolVar();
            tmpVars.at(row).push_back(tmp);

            if (bm.at(row).at(col)) {
                model.AddEquality(result.at(row), tmp);
                hasOne = 1;
            }
        }

        if (!hasOne && !isShuffle) model.AddEquality(result.at(row), model.FalseVar());
    }
    for (int col = 0; col < inLen; ++col) {
        std::vector<BoolVar> tmpSumVars{{model.TrueVar(), bv.at(col)}};

        for (int row = 0; row < outLen; ++row)
            if (bm.at(row).at(col))
                tmpSumVars.push_back(tmpVars.at(row).at(col));

        model.AddBoolXor(tmpSumVars);
    }

    return result;
}

auto linear_bmXbv_reserved(sat::CpModelBuilder &model, const std::vector<std::vector<int>> &bm, BoolVec &bv, const bool isShuffle = false) -> std::tuple<BoolVec, BoolVec> {
    auto copies = linear_bv_copy(model, bv, 2);
    auto result = linear_bmXbv_consumed(model, bm, copies.at(1), isShuffle);
    return {result, copies.at(0)};
}

#endif