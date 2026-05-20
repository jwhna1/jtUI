// Package: hui::theme::vscode
// Author:   jtai团队 :（曾能混&tang先森）<jwhna1@gmail.com>
// https://jtai.cc
#include "hui/theme/vscode_tokens.hpp"

namespace hui::theme::vscode {

namespace {

constexpr VsBase make_base() {
    return VsBase {
        .base_01 = Color::from_hex("#FFFFFF"),
        .base_02 = Color::from_hex("#F0F0F0"),
        .base_03 = Color::from_hex("#E7E7E7"),
        .base_04 = Color::from_hex("#E5E5E5"),
        .base_05 = Color::from_hex("#D4D4D4"),
        .base_06 = Color::from_hex("#CCCCCC"),
        .base_07 = Color::from_hex("#C6C6C6"),
        .base_08 = Color::from_hex("#BBBBBB"),
        .base_09 = Color::from_hex("#A0A0A0"),
        .base_10 = Color::from_hex("#808080"),
        .base_11 = Color::from_hex("#7F7F7F"),
        .base_12 = Color::from_hex("#606060"),
        .base_13 = Color::from_hex("#454545"),
        .base_14 = Color::from_hex("#3C3C3C"),
        .base_15 = Color::from_hex("#3A3D41"),
        .base_16 = Color::from_hex("#333333"),
        .base_17 = Color::from_hex("#303031"),
        .base_18 = Color::from_hex("#292929"),
        .base_19 = Color::from_hex("#252526"),
        .base_20 = Color::from_hex("#1E1E1E"),
        .base_21 = Color::from_hex("#000000"),
    };
}

constexpr VsAccent make_accent() {
    return VsAccent {
        .blue = {
            .blue_01 = Color::from_hex("#75BEFF"),
            .blue_02 = Color::from_hex("#40A6FF"),
            .blue_03 = Color::from_hex("#3399CC"),
            .blue_04 = Color::from_hex("#3794FF"),
            .blue_05 = Color::from_hex("#0097FB"),
            .blue_06 = Color::from_hex("#007ACC"),
            .blue_07 = Color::from_hex("#0E639C"),
            .blue_08 = Color::from_hex("#264F78"),
            .blue_09 = Color::from_hex("#094771"),
            .blue_10 = Color::from_hex("#062F4A"),
            .blue_11 = Color::from_hex("#001F33"),
        },
        .red = {
            .red_01 = Color::from_hex("#F14C4C"),
            .red_02 = Color::from_hex("#C74E39"),
            .red_03 = Color::from_hex("#FF0000"),
            .red_04 = Color::from_hex("#264F78"),
        },
        .green = {
            .green_01 = Color::from_hex("#73C991"),
            .green_02 = Color::from_hex("#40C8AE"),
            .green_03 = Color::from_hex("#16825D"),
            .green_04 = Color::from_hex("#327E36"),
            .green_05 = Color::from_hex("#28632B"),
        },
        .purple = {
            .purple_01 = Color::from_hex("#6C6CC4"),
            .purple_02 = Color::from_hex("#B180D7"),
            .purple_03 = Color::from_hex("#BC3FBC"),
            .purple_04 = Color::from_hex("#68217A"),
        },
        .yellow = {
            .yellow_01 = Color::from_hex("#D7BA7D"),
            .yellow_02 = Color::from_hex("#CCA700"),
            .yellow_03 = Color::from_hex("#B89500"),
            .yellow_04 = Color::from_hex("#BF8803"),
            .yellow_05 = Color::from_hex("#FFFF00"),
        },
        .orange = {
            .orange_01 = Color::from_hex("#CC6633"),
            .orange_02 = Color::from_hex("#EE9D28"),
            .orange_03 = Color::from_hex("#EA5C00"),
        },
        .gray = {
            .gray_01 = Color::from_hex("#E4E6F1"),
            .gray_02 = Color::from_hex("#5F6A79"),
            .gray_03 = Color::from_hex("#424750"),
        },
    };
}

constexpr VsSeti make_seti() {
    return VsSeti {
        .blue   = Color::from_hex("#519ABA"),
        .green  = Color::from_hex("#8DC149"),
        .orange = Color::from_hex("#E37933"),
        .pink   = Color::from_hex("#F55385"),
        .red    = Color::from_hex("#CC3E44"),
        .steel  = Color::from_hex("#7494A3"),
        .yellow = Color::from_hex("#CBCB41"),
        .purple = Color::from_hex("#A074C4"),
        .ignore = Color::from_hex("#41535B"),
        .white  = Color::from_hex("#D4D7D6"),
        .gray   = Color::from_hex("#6D8086"),
    };
}

constexpr VsPalette kPalette {
    .base   = make_base(),
    .accent = make_accent(),
    .seti   = make_seti(),
};

}  // namespace

const VsPalette& palette() noexcept {
    return kPalette;
}

}  // namespace hui::theme::vscode
