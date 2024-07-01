SUBSCRIBER(g:ymake)

PY23_TEST()

IF (PY2)
    TEST_SRCS(
        build_catboost.py
        collect_java_srcs.py
        compile_cuda.py
        coverage-info.py
        copy_clang_profile_rt.py
        create_jcoverage_report.py
        custom_link_green_mysql.py
        f2c.py
        fetch_from.py
        fetch_from_archive.py
        fetch_from_mds.py
        fetch_from_npm.py
        fetch_from_sandbox.py
        fetch_resource.py
        fix_py2_protobuf.py
        gen_java_codenav_entry.py
        gen_py3_reg.py
        go_tool.py
        ios_wrapper.py
        link_dyn_lib.py
        mangle_typeinfo_names.py
        pack_ios.py
        pack_jcoverage_resources.py
        python_yndexer.py
        run_ios_simulator.py
        run_msvc_wine.py
        run_sonar.py
        symlink.py
        touch.py
        unpacking_jtest_runner.py
        vcs_info.py
        with_crash_on_timeout.py
        yndexer.py
    )
ELSEIF (PY3)
    STYLE_PYTHON()
    TEST_SRCS(
        append_file.py
        autotar_gendirs.py
        build_dll_and_java.py
        build_info_gen.py
        build_java_codenav_index.py
        build_java_with_error_prone2.py
        cat.py
        cgo1_wrapper.py
        check_config_h.py
        clang_tidy.py
        clang_tidy_arch.py
        clang_wrapper.py
        compile_java.py
        compile_jsrc.py
        compile_pysrc.py
        configure_file.py
        container.py
        copy_docs_files.py
        copy_docs_files_to_dir.py
        copy_files_to_dir.py
        copy_to_dir.py
        cpp_flatc_wrapper.py
        decimal_md5.py
        error.py
        extract_asrc.py
        extract_docs.py
        extract_jacoco_report.py
        fail_module_cmd.py
        filter_zip.py
        find_and_tar.py
        find_time_trace.py
        fix_java_command_file_cp.py
        fix_msvc_output.py
        fix_py2_protobuf.py
        fs_tools.py
        gen_aar_gradle_script.py
        gen_java_codenav_protobuf.py
        gen_join_srcs.py
        gen_py_protos.py
        gen_py_reg.py
        gen_swiftc_output_map.py
        gen_tasklet_reg.py
        gen_test_apk_gradle_script.py
        gen_yql_python_udf.py
        generate_mf.py
        generate_pom.py
        generate_win_vfs.py
        go_proto_wrapper.py
        java_command_file.py
        java_pack_to_file.py
        jni_swig.py
        kt_copy.py
        link_asrc.py
        link_exe.py
        link_fat_obj.py
        link_jsrc.py
        link_lib.py
        list.py
        llvm_opt_wrapper.py
        make_container.py
        make_container_layer.py
        make_java_classpath_file.py
        make_java_srclists.py
        make_manifest_from_bf.py
        merge_coverage_data.py
        merge_files.py
        mkdir.py
        mkdocs_builder_wrapper.py
        mkver.py
        move.py
        postprocess_go_fbs.py
        preprocess.py
        process_command_files.py
        process_whole_archive_option.py
        py_compile.py
        resolve_java_srcs.py
        retry.py
        rodata2asm.py
        rodata2cpp.py
        run_javac.py
        run_junit.py
        run_llvm_dsymutil.py
        run_tool.py
        setup_java_tmpdir.py
        sky.py
        stderr2stdout.py
        stdout2stderr.py
        tar_directory.py
        tar_sources.py
        tared_protoc.py
        with_coverage.py
        with_kapt_args.py
        with_pathsep_resolve.py
        wrap_groovyc.py
        wrapcc.py
        wrapper.py
        write_file_size.py
        writer.py
        xargs.py
        yield_line.py
    )
ENDIF()

END()
