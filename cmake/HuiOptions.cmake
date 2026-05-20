include_guard(GLOBAL)

function(hui_configure_global_options)
    if(WIN32)
        add_compile_definitions(
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS
        )
    endif()
endfunction()

function(hui_configure_target target)
    target_compile_features(${target} PUBLIC cxx_std_20)
    hui_enable_warnings(${target})
endfunction()
