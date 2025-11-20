#ifndef AES_MODEL_HPP
#define AES_MODEL_HPP

#include <vector>
#include <array>
#include <cstdint>

#include "aes.hpp"
#include "ortools_kit.hpp"

static const auto Lbm = compute_lbm();
static const auto MCbm = compute_mcbm();

inline static auto bytes2bits(const std::vector<unsigned short> bytes)
{
    std::vector<int64_t> bits;
    const auto cnt = bytes.size();

    for (int i = 0; i < cnt; ++i)
        for (int j = 0; j < 8; ++j)
            bits.push_back((bytes[i] >> (7 - j)) & 1);

    return bits;
}

[[nodiscard]]
static BoolVec mu_bv(sat::CpModelBuilder &model, BoolVec &bv)
{
    BoolVec r = NewBoolVec(model, 16);

    for (int i = 0; i < 16; ++i) {
        BoolVec tmp1;
        BoolVec tmp2;
        for (int j = 0; j < 8; ++j) {
            tmp1.push_back(bv[i * 8 + j]);
            tmp2.push_back(bv[i * 8 + j].Not());

            // MILP
            model.AddGreaterOrEqual(r[i], bv[i * 8 + j]);
        }

        model.AddBoolOr(tmp1).OnlyEnforceIf(r[i]);
        model.AddBoolAnd(tmp2).OnlyEnforceIf(r[i].Not());

        // MILP
        model.AddLessOrEqual(r[i], LinearExpr::Sum(tmp1));
    }

    return r;
}

static void add_active(sat::CpModelBuilder &model, BoolVec &x, BoolVec &y, std::vector<BoolVar> &actives, const std::vector<int> &mask)
{
    const int lx = x.size() / 8;
    for (int i = 0; i < lx; ++i) {
        BoolVec tmpIn;
        BoolVec tmpOut;
        for (int j = 0; j < 8; ++j) tmpIn.push_back(x[8 * i + j]);
        for (int j = 0; j < 8; ++j) tmpOut.push_back(y[8 * i + j]);

        BoolVec tmpInNot;
        BoolVec tmpOutNot;
        for (int j = 0; j < 8; ++j) tmpInNot.push_back(x[8 * i + j].Not());
        for (int j = 0; j < 8; ++j) tmpOutNot.push_back(y[8 * i + j].Not());

        auto isActive =  model.NewBoolVar();
        model.AddBoolOr(tmpIn).OnlyEnforceIf(isActive);
        model.AddBoolAnd(tmpInNot).OnlyEnforceIf(isActive.Not());
        model.AddBoolOr(tmpOut).OnlyEnforceIf(isActive);
        model.AddBoolAnd(tmpOutNot).OnlyEnforceIf(isActive.Not());

        if (mask.at(i)) actives.push_back(isActive);
    }
    return;
}

static void add_active(sat::CpModelBuilder &model, BoolVec &x, BoolVec &y, std::vector<BoolVar> &actives)
{
    const int lx = x.size() / 8;
    for (int i = 0; i < lx; ++i) {
        BoolVec tmpIn;
        BoolVec tmpOut;
        for (int j = 0; j < 8; ++j) tmpIn.push_back(x[8 * i + j]);
        for (int j = 0; j < 8; ++j) tmpOut.push_back(y[8 * i + j]);

        BoolVec tmpInNot;
        BoolVec tmpOutNot;
        for (int j = 0; j < 8; ++j) tmpInNot.push_back(x[8 * i + j].Not());
        for (int j = 0; j < 8; ++j) tmpOutNot.push_back(y[8 * i + j].Not());

        auto isActive =  model.NewBoolVar();
        model.AddBoolOr(tmpIn).OnlyEnforceIf(isActive);
        model.AddBoolAnd(tmpInNot).OnlyEnforceIf(isActive.Not());
        model.AddBoolOr(tmpOut).OnlyEnforceIf(isActive);
        model.AddBoolAnd(tmpOutNot).OnlyEnforceIf(isActive.Not());

        actives.push_back(isActive);
    }
    return;
}

static void add_active(sat::CpModelBuilder &model, BoolVec &x, BoolVec &y)
{
    const int lx = x.size() / 8;
    for (int i = 0; i < lx; ++i) {
        BoolVec tmpIn;
        BoolVec tmpOut;
        for (int j = 0; j < 8; ++j) tmpIn.push_back(x[8 * i + j]);
        for (int j = 0; j < 8; ++j) tmpOut.push_back(y[8 * i + j]);

        BoolVec tmpInNot;
        BoolVec tmpOutNot;
        for (int j = 0; j < 8; ++j) tmpInNot.push_back(x[8 * i + j].Not());
        for (int j = 0; j < 8; ++j) tmpOutNot.push_back(y[8 * i + j].Not());

        auto isActive =  model.NewBoolVar();
        model.AddBoolOr(tmpIn).OnlyEnforceIf(isActive);
        model.AddBoolAnd(tmpInNot).OnlyEnforceIf(isActive.Not());
        model.AddBoolOr(tmpOut).OnlyEnforceIf(isActive);
        model.AddBoolAnd(tmpOutNot).OnlyEnforceIf(isActive.Not());
    }
    return;
}

static void add_simple_l(sat::CpModelBuilder &model, BoolVec &bv, BoolVec &r)
{
    auto muBv = mu_bv(model, bv);
    auto muR = mu_bv(model, r);
    for (int i = 0; i < 4; ++i) {
        BoolVec tmp;
        auto d = model.NewBoolVar();
        for (int j = 0; j < 4; ++j) {
            model.AddGreaterOrEqual(d, muBv[SRp[4 * i + j]]);
            model.AddGreaterOrEqual(d, muR[4 * i + j]);
            tmp.push_back(muBv[SRp[4 * i + j]]);
            tmp.push_back(muR[4 * i + j]);
        }
        model.AddGreaterOrEqual(LinearExpr::Sum(tmp), 5 * d);
    }
    return;
}

static void add_simple_mc(sat::CpModelBuilder &model, BoolVec &bv, BoolVec &r)
{
    auto muBv = mu_bv(model, bv);
    auto muR = mu_bv(model, r);
    for (int i = 0; i < 4; ++i) {
        BoolVec tmp;
        auto d = model.NewBoolVar();
        for (int j = 0; j < 4; ++j) {
            model.AddGreaterOrEqual(d, muBv[4 * i + j]);
            model.AddGreaterOrEqual(d, muR[4 * i + j]);
            tmp.push_back(muBv[4 * i + j]);
            tmp.push_back(muR[4 * i + j]);
        }
        model.AddGreaterOrEqual(LinearExpr::Sum(tmp), 5 * d);
    }
    return;
}

void add_lat(sat::CpModelBuilder &model, BoolVec &x, BoolVec &y)
{
    static auto lat = compute_lat();

    std::vector<std::vector<int64_t>> values;
    for (int i = 0; i < 256; ++i)
        for (int j = 0; j < 256; ++j)
            if (lat[i][j]) {
                values.push_back(bytes2bits({static_cast<unsigned short>(i), static_cast<unsigned short>(j)}));
            }

    const int lx = x.size() / 8;
    for (int i = 0; i < lx; ++i) {
        BoolVec tmp;

        for (int j = 0; j < 8; ++j) tmp.push_back(x[8 * i + j]);
        for (int j = 0; j < 8; ++j) tmp.push_back(y[8 * i + j]);

        BVAssign(model, tmp, values);
    }
    return;
}

void add_ilat(sat::CpModelBuilder &model, BoolVec &x, BoolVec &y)
{
    static auto ilat = compute_ilat();

    std::vector<std::vector<int64_t>> values;
    for (int i = 0; i < 256; ++i)
        for (int j = 0; j < 256; ++j)
            if (ilat[i][j]) {
                values.push_back(bytes2bits({static_cast<unsigned short>(i), static_cast<unsigned short>(j)}));
            }

    const int lx = x.size() / 8;
    for (int i = 0; i < lx; ++i) {
        BoolVec tmp;

        for (int j = 0; j < 8; ++j) tmp.push_back(x[8 * i + j]);
        for (int j = 0; j < 8; ++j) tmp.push_back(y[8 * i + j]);

        BVAssign(model, tmp, values);
    }
    return;
}

#endif