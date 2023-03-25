int regression_main(int argc, char *argv[], init_function ifunc,
                    test_function tfunc, diag_function dfunc) {
#ifdef BUILD_BY_CMAKE
    get_value_from_env(&code_base_src,"CODE_BASE_SRC");
    get_value_from_cwd(&current_exe_dir);
    get_value_from_env(&CMAKE_PGBINDIR,"GAUSSHOME");
    get_value_from_env(&CMAKE_LIBDIR,"GAUSSHOME");
    get_value_from_env(&CMAKE_PGSHAREDIR,"GAUSSHOME");

header(_("start fastcheck"));
printf("--------------------------------------------------------------------------------------");
printf("\ncode_base_src:%s\n",code_base_src);
printf("\ncurrent_exe_dir:%s\n",current_exe_dir);
printf("\nCMAKE_PGBINDIR:%s\n",CMAKE_PGBINDIR);
printf("\nCMAKE_LIBDIR:%s\n",CMAKE_LIBDIR);
printf("\nCMAKE_PGSHAREDIR:%s\n",CMAKE_PGSHAREDIR);

#endif

    _stringlist* ssl = NULL;
    int c;
    int i;
    int option_index;
    char buf[MAXPGPATH * 4];

    int coordnode_num = 3;
    int datanode_num = 12;
    int init_port = 25632;
    bool keep_last_time_data = false;
    bool run_test_case = true;
    bool run_qunit = false;
    char* qunit_module = "all";
    char* qunit_level = "all";
    bool override_installdir = false;
    bool only_install = false;
    int kk = 0;
    int patch_number = 0;
    char patch_file[MAX_PATCH_NUMBER][MAX_SIZE_OF_PATCH_NAME] = {0};

    struct timeval start_time;
    struct timeval end_time;
    double total_time;

    struct timeval start_time_total;
    struct timeval end_time_total;

    int ret;

    (void)gettimeofday(&start_time_total, NULL);

    static struct option long_options[] = {{"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'V'},
        {"dbname", required_argument, NULL, 1},
        {"debug", no_argument, NULL, 2},
        {"inputdir", required_argument, NULL, 3},
        {"load-language", required_argument, NULL, 4},
        {"max-connections", required_argument, NULL, 5},
        {"encoding", required_argument, NULL, 6},
        {"outputdir", required_argument, NULL, 7},
        {"schedule", required_argument, NULL, 8},
        {"temp-install", required_argument, NULL, 9},
        {"no-locale", no_argument, NULL, 10},
        {"top-builddir", required_argument, NULL, 11},
        {"host", required_argument, NULL, 13},
        {"port", required_argument, NULL, 14},
        {"user", required_argument, NULL, 15},
        {"psqldir", required_argument, NULL, 16},
        {"dlpath", required_argument, NULL, 17},
        {"create-role", required_argument, NULL, 18},
        {"temp-config", required_argument, NULL, 19},
        {"use-existing", no_argument, NULL, 20},
        {"launcher", required_argument, NULL, 21},
        {"load-extension", required_argument, NULL, 22},
        {"extra-install", required_argument, NULL, 23},
        {"noclean", no_argument, NULL, 24},
        {"regconf", required_argument, NULL, 25},
        {"hdfshostname", required_argument, NULL, 26},
        {"hdfsport", required_argument, NULL, 27},
        {"hdfscfgpath", required_argument, NULL, 28},
        {"hdfsstoreplus", required_argument, NULL, 29},
        {"obshostname", required_argument, NULL, 30},
        {"obsbucket", required_argument, NULL, 31},
        {"keep_last_data", required_argument, NULL, 32},
        {"securitymode", no_argument, NULL, 33},
        {"abs_gausshome", required_argument, NULL, 34},
        {"ak", required_argument, NULL, 35},
        {"sk", required_argument, NULL, 36},
        {"inplace_upgrade", no_argument, NULL, 37},
        {"init_database", no_argument, NULL, 38},
        {"data_base_dir", required_argument, NULL, 39},
        {"upgrade_from", required_argument, NULL, 40},
        {"upgrade_script_dir", required_argument, NULL, 41},
        {"ignore-exitcode", no_argument, NULL, 42},
        {"keep-run", no_argument, NULL, 43},
        {"parallel_initdb", no_argument, NULL, 44},
        {"single_node", no_argument, NULL, 45},
        {"qunit", no_argument, NULL, 46},
        {"qunit_module", required_argument, NULL, 47},
        {"qunit_level", required_argument, NULL, 48},
        {"old_bin_dir", required_argument, NULL, 49},
        {"grayscale_mode", no_argument, NULL, 50},
        {"grayscale_full_mode", no_argument, NULL, 51},
        {"uc", required_argument, NULL, 52},
        {"ud", required_argument, NULL, 53},
        {"upgrade_schedule", required_argument, NULL, 54},
        {"platform", required_argument, NULL, 55},
        {"seprate_unix_socket_dir", no_argument, NULL, 56},
        {"bucket_len", required_argument, NULL, 57},
        {"check_hotpatch", no_argument, NULL, 58},
        {"patch_dir", required_argument, NULL, 59},
        {"enable_segment", no_argument, NULL, 60},
        {"client_logic_hook", required_argument, NULL, 61},
        {"jdbc", no_argument, NULL, 62},
        {"skip_environment_cleanup", no_argument, NULL, 63},
        {"enable_security_policy", no_argument, NULL, 64},
        {"ecpg", no_argument, NULL, 65},
        {NULL, 0, NULL, 0}
    };

    progname = get_progname(argv[0]);
    set_pglocale_pgservice(argv[0], PG_TEXTDOMAIN("pg_regress"));

#ifndef HAVE_UNIX_SOCKETS
    /* no unix domain sockets available, so change default */
    hostname = "localhost";
#endif

    /*
     * We call the initialization function here because that way we can set
     * default parameters and let them be overwritten by the commandline.
     */
    ifunc();

    while ((c = getopt_long(argc, argv, "b:c:d:hp:r:s:V:n:w", long_options, &option_index)) != -1) {
        switch (c) {
            case 'h':
                help();
                exit_nicely(0);
                /* fall through */
            case 'V':
                puts("pg_regress " DEF_GS_VERSION);
                exit_nicely(0);
                /* fall through */
            case 'd':
                datanode_num = atoi(optarg);
                break;
            case 'c':
                coordnode_num = atoi(optarg);
                break;
            case 'p':
                init_port = atoi(optarg);
                g_gdsPort = init_port - 1;
                /*
                 * To guaranteeing the port number bigger than 9999.
                 */
                if (g_gdsPort > 1023 && g_gdsPort <= 5000) {
                    g_gdsPort = g_gdsPort * 10;
                }
                if (g_gdsPort > 5000 && g_gdsPort < 10000) {
                    g_gdsPort = g_gdsPort * 4;
                }
                break;
            case 'r':
                kk = atoi(optarg);
                if (kk == 2) {
                    only_install = true;
                    run_test_case = false;
                } else if (kk == 0)
                    run_test_case = false;
                else
                    run_test_case = true;

                break;
            case 'b':
                temp_install = make_absolute_path(optarg);
                override_installdir = true;
                break;
            case 'm':
                if (strncmp(optarg, "tcp", 3) != 0)
                    comm_tcp_mode = false;
                break;
            case 's':
                standby_defined = atoi(optarg);
                break;
            case 'w':
                change_password = true;
                break;
            case 'n':
                dop = atoi(optarg);
                break;
            case 1:

                /*
                 * If a default database was specified, we need to remove it
                 * before we add the specified one.
                 */
                free_stringlist(&dblist);
                split_to_stringlist(strdup(optarg), ", ", &dblist);
                break;
            case 2:
                debug = true;
                break;
            case 3:
                inputdir = strdup(optarg);
                break;
            case 4:
                add_stringlist_item(&loadlanguage, optarg);
                break;
            case 5:
                max_connections = atoi(optarg);
                break;
            case 6:
                encoding = strdup(optarg);
                break;
            case 7:
                outputdir = strdup(optarg);
                break;
            case 8:
                add_stringlist_item(&schedulelist, optarg);
                break;
            case 9:
                if (override_installdir == false)
                    temp_install = make_absolute_path(optarg);
                break;
            case 10:
                nolocale = true;
                break;
            case 11:
                top_builddir = strdup(optarg);
                break;
            case 13:
                hostname = strdup(optarg);
                break;
            case 14:
                port = atoi(optarg);
                port_specified_by_user = true;
                break;
            case 15:
                user = strdup(optarg);
                break;
            case 16:
                /* "--psqldir=" should mean to use PATH */
                if (strlen(optarg)) {
                    psqldir = strdup(optarg);
                }
                break;
            case 17:
                dlpath = strdup(optarg);
                break;
            case 18:
                split_to_stringlist(strdup(optarg), ", ", &extraroles);
                break;
            case 19:
                temp_config = strdup(optarg);
                break;
            case 20:
                use_existing = true;
                break;
            case 21:
                launcher = strdup(optarg);
                break;
            case 22:
                add_stringlist_item(&loadextension, optarg);
                break;
            case 23:
                add_stringlist_item(&extra_install, optarg);
                break;
            case 24:
                clean = false;
                break;
            case 25:
                pcRegConfFile = strdup(optarg);
                break;
            case 26:
                hdfshostname = strdup(optarg);
                break;
            case 27:
                hdfsport = strdup(optarg);
                break;
            case 28:
                hdfscfgpath = strdup(optarg);
                break;
            case 29:
                hdfsstoreplus = strdup(optarg);
                break;
            case 30:
                obshostname = strdup(optarg);
                break;
            case 31:
                obsbucket = strdup(optarg);
                break;
            case 32: {
                char* tmp_str = NULL;
                tmp_str = strdup(optarg);
                if (pg_strncasecmp(tmp_str, "true", 4) == 0) {
                    keep_last_time_data = true;
                } else {
                    keep_last_time_data = false;
                }
                free(tmp_str);
                break;
            }
            case 33:
                securitymode = true;
                break;
            case 34:
                gausshomedir = strdup(optarg);
                break;
            case 35:
                ak = strdup(optarg);
                break;
            case 36:
                sk = strdup(optarg);
                break;
            case 37:
                inplace_upgrade = true;
                super_user_altered = false;
                passwd_altered = false;
                break;
            case 38:
                init_database = true;
                break;
            case 39:
                data_base_dir = strdup(optarg);
                break;
            case 40:
                upgrade_from = atoi(optarg);
                break;
            case 41:
                upgrade_script_dir = strdup(optarg);
                break;
            case 42:
                ignore_exitcode = true;
                break;
            case 43:
                keep_run = true;
                break;
            case 44:
                parallel_initdb = true;
                break;
            case 45: {
                /* test single datanode mode */
                test_single_node = true;
                break;
            }
            case 46:
                run_qunit = true;
                break;
            case 47:
                qunit_module = strdup(optarg);
                break;
            case 48:
                qunit_level = strdup(optarg);
                break;
            case 49:
                old_bin_dir = strdup(optarg);
                break;
            case 50:
                grayscale_upgrade = 0;
                super_user_altered = false;
                passwd_altered = false;
                break;
            case 51:
                grayscale_upgrade = 1;
                super_user_altered = false;
                passwd_altered = false;
                break;
            case 52:
                upgrade_cn_num = atoi(optarg);
                if (upgrade_cn_num > 2 || upgrade_cn_num < 1) {
                    upgrade_cn_num = 1;
                }
                break;
            case 53:
                upgrade_dn_num = atoi(optarg);
                if (upgrade_dn_num > 11 || upgrade_dn_num < 1) {
                    upgrade_dn_num = 4;
                }
                break;
            case 54:
                add_stringlist_item(&upgrade_schedulelist, optarg);
                break;
            case 55:
                platform = strdup(optarg);
                break;
            case 56:
                seprate_unix_socket_dir = true;
                break;
            case 57: {
                int bucketLen = atoi(optarg);
                ret = snprintf_s(bucketLenStr, sizeof(bucketLenStr),
                                 sizeof(bucketLenStr) - 1, "--bucketlength=%d", bucketLen);
                securec_check_ss_c(ret, "", "");
                break;
            }
            case 58:
                check_hotpatch = true;
                break;
            case 59:
                dir_hotpatch = make_absolute_path(optarg);
                break;
            case 60:
                enable_segment = true;
                break;
            case 61:
                client_logic_hook = make_absolute_path(optarg);
                break;
            case 62:
                printf("\n starting with jdbc\n");
                use_jdbc_client = true;
                to_create_jdbc_user = true;
                break;
            case 63:
                is_skip_environment_cleanup = true;
                break;
            case 64: /* security policy flag, increment as option param */
                printf("\n starting security policy test\n");
                enable_security_policy = true;
                break;
            case 65:
                printf("\nstarting ecpg test\n");
                use_ecpg = true;
                break;
            default:
                /* getopt_long already emitted a complaint */
                fprintf(stderr, _("\nTry \"%s -h\" for more information.\n"), progname);
                exit_nicely(2);
        }
    }

    if (run_test_case && run_qunit) {
        fprintf(stderr, _("Can not run qunit and other check simultaneously\n"));
        exit_nicely(2);
    }

    memset(g_stRegrConfItems.acFieldSepForAllText, '\0', sizeof(g_stRegrConfItems.acFieldSepForAllText));
    memset(g_stRegrConfItems.acTuplesOnly, '\0', sizeof(g_stRegrConfItems.acTuplesOnly));

    regrInitReplcPattStruct();

    if (pcRegConfFile)
        loadRegressConf(pcRegConfFile);

    int result = initialize_myinfo(coordnode_num, datanode_num, init_port, keep_last_time_data, run_test_case);
    if (result != 0) {
        return -1;
    }

    // Upgrade check option
    (void)check_upgrade_options();

    /*
     * if we still have arguments, they are extra tests to run
     */
    while (argc - optind >= 1) {
        add_stringlist_item(&extra_tests, argv[optind]);
        optind++;
    }

    if (temp_install && !port_specified_by_user)
    /*
     * To reduce chances of interference with parallel installations, use
     * a port number starting in the private range (49152-65535)
     * calculated from the version number.
     */
    {
        port = 0xC000 | (PG_VERSION_NUM & 0x3FFF);
    }

    inputdir = make_absolute_path(inputdir);
    outputdir = make_absolute_path(outputdir);
    dlpath = make_absolute_path(dlpath);
    data_base_dir = make_absolute_path(data_base_dir);
    old_bin_dir = make_absolute_path(old_bin_dir);
    upgrade_script_dir = make_absolute_path(upgrade_script_dir);
    loginuser = get_id();

#if defined (__x86_64__)
    ret = snprintf_s(pgbenchdir, MAXPGPATH, MAXPGPATH - 1, "%s/%s", ".", "data/pgbench/x86_64");
    securec_check_ss_c(ret, "", "");
#elif defined (__aarch64__)
    ret = snprintf_s(pgbenchdir, MAXPGPATH, MAXPGPATH - 1, "%s/%s", ".", "data/pgbench/aarch64");
    securec_check_ss_c(ret, "", "");
#endif

    /* Check thread local varieble's num */
    check_global_variables();

    /* Check marcos like STREAMPLAN/PGXC/IS_SINGLE_NODE */
    check_pgxc_like_macros();

    // Do not stop postmaster if we are told not to run test cases.
    if (run_test_case)
        (void)atexit(atexit_cleanup);

    /*
     * create regression.out regression.diff result
     */
    open_result_files();
    // Initialize environment variables, including but not limited to LD_LIBRARY_PATH, PATH
    initialize_environment();

#if defined(HAVE_GETRLIMIT) && defined(RLIMIT_CORE)
    unlimit_core_size();
#endif

    // temp_install != NULL; Whether to specify the installation directory
    if (temp_install) {
        /*
         * Prepare the temp installation
         */
        if (!top_builddir) {
            fprintf(stderr, _("--top-builddir must be specified when using --temp-install\n"));
            exit_nicely(2);
        }

        if (!directory_exists(temp_install)) {
            myinfo.keep_data = false;
        }
#ifndef ENABLE_LLT
        if (myinfo.keep_data) {
            //	header(_("removing existing gtm file"));
            //	rmtree(temp_gtm_install, true);

            /* "make install" */
#ifndef WIN32_ONLY_COMPILER
            if (!check_hotpatch) {
                ret = snprintf_s(buf,
                    sizeof(buf),
                    sizeof(buf) - 1,
                    SYSTEMQUOTE
                    "\"%s\" -C \"%s\" DESTDIR=\"%s/install\" install -sj > \"%s/log/install.log\" 2>&1" SYSTEMQUOTE,
                    makeprog,
                    top_builddir,
                    temp_install,
                    outputdir);
                securec_check_ss_c(ret, "", "");
            }
#else
            ret = snprintf_s(buf,
                sizeof(buf),
                sizeof(buf) - 1,
                SYSTEMQUOTE
                "perl \"%s/src/tools/msvc/install.pl\" \"%s/install\" >\"%s/log/install.log\" 2>&1" SYSTEMQUOTE,
                top_builddir,
                temp_install,
                outputdir);
            securec_check_ss_c(ret, "", "");
#endif
            if (system(buf)) {
                fprintf(stderr,
                    _("\n%s: installation failed\nExamine %s/log/install.log for the reason.\nCommand was: %s\n"),
                    progname,
                    outputdir,
                    buf);
                exit_nicely(2);
            }
        }
#endif
        if (myinfo.keep_data == false) {
            if (only_install == false && (check_hotpatch != true)) {
#ifndef ENABLE_LLT
                if (directory_exists(temp_install)) {
                    header(_("removing existing temp installation"));
                    (void)rmtree(temp_install, true);
                }

                header(_("creating temporary installation"));

                /* make the temp install top directory */
                make_directory(temp_install);
#endif

                /* and a directory for log files */
                ret = snprintf_s(buf, sizeof(buf), sizeof(buf) - 1, "%s/log", outputdir);
                securec_check_ss_c(ret, "", "");
                if (!directory_exists(buf)) {
                    make_directory(buf);
                }

#ifdef ENABLE_LLT
                if (directory_exists(temp_install)) {
                    ret = snprintf_s(buf,
                        sizeof(buf),
                        sizeof(buf) - 1,
                        SYSTEMQUOTE "rm -rf %s/coordinator* > %s/log/install.log 2>&1" SYSTEMQUOTE,
                        temp_install,
                        outputdir);
                    securec_check_ss_c(ret, "", "");
                    header("%s", buf);
                    if (system(buf)) {
                        fprintf(stderr,
                            _("\n%s: installation failed\nExamine %s/log/install.log for the reason.\nCommand was: "
                              "%s\n"),
                            progname,
                            outputdir,
                            buf);
                        exit_nicely(2);
                    }

                    ret = snprintf_s(buf,
                        sizeof(buf),
                        sizeof(buf) - 1,
                        SYSTEMQUOTE "rm -rf %s/data* > %s/log/install.log 2>&1" SYSTEMQUOTE,
                        temp_install,
                        outputdir);
                    securec_check_ss_c(ret, "", "");
                    header("%s", buf);
                    if (system(buf)) {
                        fprintf(stderr,
                            _("\n%s: installation failed\nExamine %s/log/install.log for the reason.\nCommand was: "
                              "%s\n"),
                            progname,
                            outputdir,
                            buf);
                        exit_nicely(2);
                    }
                }
#endif
            }
#ifndef ENABLE_LLT
            /* "make install" */

#ifdef BUILD_BY_CMAKE
            ret = snprintf_s(buf,
                sizeof(buf),
                sizeof(buf) - 1,
                SYSTEMQUOTE
                "cd %s && \"%s\" DESTDIR=\"%s/install\" install -j >> \"%s/log/install.log\" 2>&1" SYSTEMQUOTE,
                current_exe_dir, makeprog, temp_install, outputdir);
            securec_check_ss_c(ret, "", "");
            printf("---------------------------------make install---------------------------------\n");
            printf("cd %s && \"%s\" DESTDIR=\"%s/install\" install -j >> \"%s/log/install.log\" 2>&1\n",
                   current_exe_dir, makeprog, temp_install, outputdir);
#elif !defined(WIN32_ONLY_COMPILER)
            ret = snprintf_s(buf,
                sizeof(buf),
                sizeof(buf) - 1,
                SYSTEMQUOTE
                "\"%s\" -C \"%s\" DESTDIR=\"%s/install\" install -sj > \"%s/log/install.log\" 2>&1" SYSTEMQUOTE,
                makeprog,
                top_builddir,
                temp_install,
                outputdir);
            securec_check_ss_c(ret, "", "");
#else
            ret = snprintf_s(buf,
                sizeof(buf),
                sizeof(buf) - 1,
                SYSTEMQUOTE
                "perl \"%s/src/tools/msvc/install.pl\" \"%s/install\" >\"%s/log/install.log\" 2>&1" SYSTEMQUOTE,
                top_builddir,
                temp_install,
                outputdir);
            securec_check_ss_c(ret, "", "");
#endif
            if (system(buf)) {
                fprintf(stderr,
                    _("\n%s: installation failed\nExamine %s/log/install.log for the reason.\nCommand was: %s\n"),
                    progname,
                    outputdir,
                    buf);
                exit_nicely(2);
            } else if (only_install) {
                exit_nicely(2);
            }

            for (ssl = extra_install; ssl != NULL; ssl = ssl->next) {
#ifndef WIN32_ONLY_COMPILER
                ret = snprintf_s(buf,
                    sizeof(buf),
                    sizeof(buf) - 1,
                    SYSTEMQUOTE
                    "\"%s\" -C \"%s/%s\" DESTDIR=\"%s/install\" install >> \"%s/log/install.log\" 2>&1" SYSTEMQUOTE,
                    makeprog,
                    top_builddir,
                    ssl->str,
                    temp_install,
                    outputdir);
                securec_check_ss_c(ret, "", "");
#else
                fprintf(stderr, _("\n%s: --extra-install option not supported on this platform\n"), progname);
                exit_nicely(2);
#endif
                if (system(buf)) {
                    fprintf(stderr,
                        _("\n%s: installation failed\nExamine %s/log/install.log for the reason.\nCommand was: %s\n"),
                        progname,
                        outputdir,
                        buf);
                    exit_nicely(2);
                }
            }
#endif
            if ((!inplace_upgrade && grayscale_upgrade == -1) || init_database) {
                /* initdb */
                header(_("initializing database system"));
                init_gtm();
                /* Initialize nodes */
                if (parallel_initdb)
                    initdb_node_info_parallel(standby_defined);
                else
                    initdb_node_info(standby_defined);
            } else {
                (void)gettimeofday(&start_time, NULL);
                /* copy base data and binary for upgrade */
                ret = snprintf_s(buf,
                    sizeof(buf),
                    sizeof(buf) - 1,
                    SYSTEMQUOTE "rm -rf %s/data* && rm -rf %s/coordinator* &&"
                                "rm -rf %s/data_base && rm -rf %s/coordinator* &&"
                                " rm -rf %s/bin/ && rm -rf %s/etc/ && rm -rf %s/include/ &&"
                                " rm -rf %s/jdk/ && rm -rf %s/lib/ && rm -rf %s/share/ &&"
                                " tar -xpf %s/%s/data_base.tar.gz -C %s &&"
                                " tar -xpf %s/%s/bin_base.tar.gz -C %s " SYSTEMQUOTE,
                    temp_install,
                    temp_install,
                    data_base_dir,
                    data_base_dir,
                    temp_install,
                    temp_install,
                    temp_install,
                    temp_install,
                    temp_install,
                    temp_install,
                    data_base_dir,
                    platform,
                    temp_install,
                    data_base_dir,
                    platform,
                    temp_install);
                securec_check_ss_c(ret, "", "");
                header("%s", buf);
                if (system(buf)) {
                    fprintf(stderr, _("Failed to copy base data for upgrade.\nCommand was: %s\n"), buf);
                    exit_nicely(2);
                }
                // The old binary installation is complete, switch to the old binary environment variable
                setBinAndLibPath(true);
                (void)gettimeofday(&end_time, NULL);
                total_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) * 0.000001;
                printf("Decompressing the baseline package total time: %fs\n", total_time);
            }

            /* If only init database for inplace upgrade, we are done. */
            if (init_database) { 
                cleanup_environment();
                return 0;                        
            }
                

            /*
             * Adjust the default postgresql.conf as needed for regression
             * testing. The user can specify a file to be appended; in any case we
             * set max_prepared_transactions to enable testing of prepared xacts.
             * (Note: to reduce the probability of unexpected shmmax failures,
             * don't set max_prepared_transactions any higher than actually needed
             * by the prepared_xacts regression test.)
             * Update configuration file of each node with user-defined options
             * and 2PC related information.
             * PGXCTODO: calculate port of GTM before setting configuration files
             */
            initdb_node_config_file(standby_defined);
        }
        /*Execute shell cmds in source files*/
        exec_cmds_from_inputfiles();

        /*
         * Start the temp postmaster: 3C12D
         */
        (void)start_postmaster(GTM_LITE_MODE);

        if (!test_single_node) {
            /* Postmaster is finally running, so set up connection information on Coordinators */
            if (myinfo.keep_data == false) {
                setup_connection_information(standby_defined);
                if (inplace_upgrade || grayscale_upgrade != -1) {
                    checkProcInsert();
                    setup_super_user();
                    super_user_altered = true;

                    // Execute the upgrade script in the old bin
                    ret = snprintf_s(buf,
                                     sizeof(buf),
                                     sizeof(buf) - 1,
                                     SYSTEMQUOTE "python %s/upgradeCheck.py -u -p %d -f %d -s %s%sgsql" SYSTEMQUOTE,
                            upgrade_script_dir,
                            get_port_number(0, COORD),
                            upgrade_from,
                            psqldir ? psqldir : "",
                            psqldir ? "/" : "");
                    securec_check_ss_c(ret, "", "");
                    header("%s", buf);
                    (void)gettimeofday(&end_time, NULL);
                    if (regr_system(buf)) {
                        fprintf(stderr, _("Failed to exec upgrade.\nCommand was: %s\n"), buf);
                        exit_nicely(2);
                    }

                    // start with new bin
                    restartPostmaster(false);

                    /* Execute the post upgrade script */
                    ret = snprintf_s(buf,
                                     sizeof(buf),
                                     sizeof(buf) - 1,
                                     SYSTEMQUOTE "python %s/upgradeCheck.py --post -p %d -f %d -s %s%sgsql" SYSTEMQUOTE,
                            upgrade_script_dir,
                            get_port_number(0, COORD),
                            upgrade_from,
                            psqldir ? psqldir : "",
                            psqldir ? "/" : "");
                    securec_check_ss_c(ret, "", "");
                    header("%s", buf);
                    (void)gettimeofday(&end_time, NULL);
                    if (regr_system(buf)) {
                        fprintf(stderr, _("Failed to exec post-upgrade.\nCommand was: %s\n"), buf);
                        exit_nicely(2);
                    }
                    if (grayscale_upgrade == 1) {
                        // Execute the post rollback script
                        ret = snprintf_s(buf,
                                         sizeof(buf),
                                         sizeof(buf) - 1,
                                         SYSTEMQUOTE "python %s/upgradeCheck.py --rollback -p %d -f %d -s %s%sgsql" SYSTEMQUOTE,
                                upgrade_script_dir,
                                get_port_number(0, COORD),
                                upgrade_from,
                                psqldir ? psqldir : "",
                                psqldir ? "/" : "");
                        securec_check_ss_c(ret, "", "");
                        header("%s", buf);
                        (void)gettimeofday(&end_time, NULL);
                        if (regr_system(buf)) {
                            fprintf(stderr, _("Failed to exec rollback.\nCommand was: %s\n"), buf);
                            exit_nicely(2);
                        }

                        // start with old bin
                        restartPostmaster(true);

                        // Execute the pre rollback script
                        ret = snprintf_s(buf,
                                         sizeof(buf),
                                         sizeof(buf) - 1,
                                         SYSTEMQUOTE "python %s/upgradeCheck.py -o -p %d -f %d -s %s%sgsql" SYSTEMQUOTE,
                                upgrade_script_dir,
                                get_port_number(0, COORD),
                                upgrade_from,
                                psqldir ? psqldir : "",
                                psqldir ? "/" : "");
                        securec_check_ss_c(ret, "", "");
                        header("%s", buf);
                        (void)gettimeofday(&end_time, NULL);
                        if (regr_system(buf)) {
                            fprintf(stderr, _("Failed to exec rollback.\nCommand was: %s\n"), buf);
                            exit_nicely(2);
                        }

                        // Execute the upgrade script in the old bin
                        ret = snprintf_s(buf,
                                         sizeof(buf),
                                         sizeof(buf) - 1,
                                         SYSTEMQUOTE "python %s/upgradeCheck.py -p %d -f %d -s %s%sgsql" SYSTEMQUOTE,
                                upgrade_script_dir,
                                get_port_number(0, COORD),
                                upgrade_from,
                                psqldir ? psqldir : "",
                                psqldir ? "/" : "");
                        securec_check_ss_c(ret, "", "");
                        header("%s", buf);
                        (void)gettimeofday(&end_time, NULL);
                        if (regr_system(buf)) {
                            fprintf(stderr, _("Failed to exec upgrade.\nCommand was: %s\n"), buf);
                            exit_nicely(2);
                        }

                        // start with new bin
                        restartPostmaster(false);
                        /* Execute the post upgrade script */
                        ret = snprintf_s(buf,
                                         sizeof(buf),
                                         sizeof(buf) - 1,
                                         SYSTEMQUOTE "python %s/upgradeCheck.py --post -p %d -f %d -s %s%sgsql" SYSTEMQUOTE,
                                upgrade_script_dir,
                                get_port_number(0, COORD),
                                upgrade_from,
                                psqldir ? psqldir : "",
                                psqldir ? "/" : "");
                        securec_check_ss_c(ret, "", "");
                        header("%s", buf);
                        (void)gettimeofday(&end_time, NULL);
                        if (regr_system(buf)) {
                            fprintf(stderr, _("Failed to exec post-upgrade.\nCommand was: %s\n"), buf);
                            exit_nicely(2);
                        }
                    }

                    header(_("shutting down postmaster"));
                    (void)stop_postmaster();

                    // End the upgrade process.
                    upgrade_from = 0;
                    initdb_upgrade_guc_config_file(standby_defined);
                    (void)start_postmaster(GTM_LITE_MODE);
                    header(_("sleeping"));
                    pg_usleep(10000000L);
                }
            } else {
                pg_usleep(2000000L);
                rebuild_node_group();

                for (ssl = dblist; ssl; ssl = ssl->next)
                    drop_database_if_exists(ssl->str);
                for (ssl = extraroles; ssl; ssl = ssl->next)
                    drop_role_if_exists(ssl->str);
                _stringlist* tmpdblist = NULL;
                _stringlist* tmprolelist = NULL;
                add_stringlist_item(&tmpdblist, "cstore_vacuum_full_db");
                add_stringlist_item(&tmpdblist, "test_sort");
                add_stringlist_item(&tmpdblist, "td_db");
                add_stringlist_item(&tmpdblist, "my_db1");
                add_stringlist_item(&tmpdblist, "td_db_char");
                add_stringlist_item(&tmpdblist, "td_db_char_cast");
                add_stringlist_item(&tmpdblist, "tdtest");
                add_stringlist_item(&tmpdblist, "td_db_char_bulkload");
                add_stringlist_item(&tmpdblist, "testaes1");
                add_stringlist_item(&tmpdblist, "db_gin_utf8");
                add_stringlist_item(&tmpdblist, "music");
                add_stringlist_item(&tmpdblist, "db_ascii_bulkload_compatibility_test");
                add_stringlist_item(&tmpdblist, "db_latin1_bulkload_compatibility_test");
                add_stringlist_item(&tmpdblist, "db_gbk_bulkload_compatibility_test");
                add_stringlist_item(&tmpdblist, "db_eucjis2004_bulkload_compatibility_test");
                add_stringlist_item(&tmpdblist, "td_format_db");
                add_stringlist_item(&tmpdblist, "test_parallel_db");
                for (ssl = tmpdblist; ssl; ssl = ssl->next)
                    drop_database_if_exists(ssl->str);

                add_stringlist_item(&tmprolelist, "temp_reset_user");
                add_stringlist_item(&tmprolelist, "cstore_role");
                add_stringlist_item(&tmprolelist, "test_llt");
                add_stringlist_item(&tmprolelist, "alter_llt2");
                add_stringlist_item(&tmprolelist, "llt_1");
                add_stringlist_item(&tmprolelist, "llt_3");
                add_stringlist_item(&tmprolelist, "llt_5");
                add_stringlist_item(&tmprolelist, "role_pwd_complex");
                add_stringlist_item(&tmprolelist, "dfm");
                add_stringlist_item(&tmprolelist, "ad1");
                add_stringlist_item(&tmprolelist, "ad2");
                add_stringlist_item(&tmprolelist, "hs");
                add_stringlist_item(&tmprolelist, "testdb_new");
                for (ssl = tmprolelist; ssl; ssl = ssl->next)
                    drop_role_if_exists(ssl->str);
            }
        }
        gen_startstop_script();
        if (check_hotpatch) {
            patch_number = list_current_dir(dir_hotpatch, patch_file, MAX_PATCH_NUMBER, MAX_SIZE_OF_PATCH_NAME);
            gen_patch_script(patch_number, patch_file);
        }
    } else {
        /*
         * Using an existing installation, so may need to get rid of
         * pre-existing database(s) and role(s)
         */
        if (myinfo.run_check == true && !use_existing) {
            for (ssl = dblist; ssl; ssl = ssl->next)
                drop_database_if_exists(ssl->str);
            for (ssl = extraroles; ssl; ssl = ssl->next)
                drop_role_if_exists(ssl->str);
        }
    }

    if (myinfo.run_check == true) {
        (void)gettimeofday(&end_time_total, NULL);
        total_time = (end_time_total.tv_sec - start_time_total.tv_sec) + (end_time_total.tv_usec - start_time_total.tv_usec) * 0.000001;
        printf("Preparation before the test case execution takes %fs..\n", total_time);
        bool use_tmp_user_dir = false;
        if (clean && run_test_case && seprate_unix_socket_dir) {
            use_tmp_user_dir = true;
        }

        if (use_tmp_user_dir) {
            restart_for_changing_unix_socket_directory();
        }

        create_data_roles(ssl);

        /*
         * Ready to run the tests
         */
        header(_("running regression test queries"));

        char *old_gausshome = NULL;
        char env_path[MAXPGPATH + sizeof("GAUSSHOME=")];

        // Query retry need env 'GAUSSHOME'.
        old_gausshome = gs_getenv_r("GAUSSHOME");
        /* keep the old shell GAUSSHOME evn */
        ret = snprintf_s(env_path, sizeof(env_path), sizeof(env_path) - 1, "OLDGAUSSHOME=%s", old_gausshome);
        securec_check_ss_c(ret, "", "");
        char *old_gausshome_env = strdup(env_path);
        gs_putenv_r(old_gausshome_env);
        /* pg regress env */
        ret = snprintf_s(env_path, sizeof(env_path), sizeof(env_path) - 1, "GAUSSHOME=%s/../", psqldir);
        securec_check_ss_c(ret, "", "");
        gs_putenv_r(env_path);

        char cgroup_cp[MAXPGPATH * 2];
        ret = snprintf_s(cgroup_cp, sizeof(cgroup_cp), sizeof(cgroup_cp) - 1, "cp %s/etc/gscgroup_*.cfg %s/../etc/",
            old_gausshome, psqldir);
        securec_check_ss_c(ret, "", "");
        system(cgroup_cp);

        (void)gettimeofday(&start_time, NULL);

        if (check_hotpatch) {
            run_patch_script();
        }

        if (!g_bEnableDiagCollection)
            dfunc = NULL;
        for (ssl = schedulelist; ssl != NULL; ssl = ssl->next) {
            run_schedule(ssl->str, tfunc, dfunc);
        }

        for (ssl = extra_tests; ssl != NULL; ssl = ssl->next) {
            run_single_test(ssl->str, tfunc, dfunc);
        }

        (void)gettimeofday(&end_time, NULL);

        // Restore the env 'GAUSSHOME'
        //
        if (old_gausshome) {
            ret = snprintf_s(env_path, sizeof(env_path), sizeof(env_path) - 1, "GAUSSHOME=%s", old_gausshome);
            securec_check_ss_c(ret, "", "");
            gs_putenv_r(env_path);
        }

        total_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) * 0.000001;

        /*
         * Shut down temp installation's postmaster
         */
        if (temp_install && clean) {
            header(_("shutting down postmaster"));
            stop_postmaster(true);
        }

        fclose(logfile);

        /*
         * Emit nice-looking summary message
         */
        if (fail_count == 0 && fail_ignore_count == 0) {
            ret = snprintf_s(buf, sizeof(buf), sizeof(buf) - 1, _(" All %d tests passed. "), success_count);
            securec_check_ss_c(ret, "", "");
        } else if (fail_count == 0) { /* fail_count=0, fail_ignore_count>0 */
            ret = snprintf_s(buf,
                sizeof(buf),
                sizeof(buf) - 1,
                _(" %d of %d tests passed, %d failed test(s) ignored. "),
                success_count,
                success_count + fail_ignore_count,
                fail_ignore_count);
            securec_check_ss_c(ret, "", "");
        } else if (fail_ignore_count == 0) { /* fail_count>0 && fail_ignore_count=0 */
            ret = snprintf_s(buf, sizeof(buf), sizeof(buf) - 1, _(" %d of %d tests failed. "),
                             fail_count, success_count + fail_count);
            securec_check_ss_c(ret, "", "");
        } else {
            /* fail_count>0 && fail_ignore_count>0 */
            ret = snprintf_s(buf,
                sizeof(buf),
                sizeof(buf) - 1,
                _(" %d of %d tests failed, %d of these failures ignored. "),
                fail_count + fail_ignore_count,
                success_count + fail_count + fail_ignore_count,
                fail_ignore_count);
            securec_check_ss_c(ret, "", "");
        }

        (void)putchar('\n');
        for (i = strlen(buf); i > 0; i--) {
            (void)putchar('=');
        }
        printf("\n%s\n", buf);
        printf(" Total Time: %fs\n", total_time);
        for (i = strlen(buf); i > 0; i--) {
            (void)putchar('=');
        }
        (void)putchar('\n');
        (void)putchar('\n');

        if (use_tmp_user_dir) {
            reset_unix_socket_directory();
        }

        if (check_hotpatch) {
            printf("Run fastcheck with hotpatch:\n");
            print_patch_list(patch_number, patch_file);
        }

        if (file_size(difffilename) > 0) {
            printf(_("The differences that caused some tests to fail can be viewed in the\n"
                     "file \"%s\".  A copy of the test summary that you see\n"
                     "above is saved in the file \"%s\".\n\n"),
                difffilename,
                logfilename);
        } else {
            unlink(difffilename);
            unlink(logfilename);
        }

        if (!keep_run && fail_count != 0) {
            exit_nicely(1);
        }
    }

    if (run_qunit == true) {
        char buf[MAXPGPATH * 2];
        int r;

        /* On Windows, system() seems not to force fflush, so... */
        fflush(stdout);
        fflush(stderr);

        ret = snprintf_s(buf,
            sizeof(buf),
            sizeof(buf) - 1,
            "../QUnit/src/qunit  -logdir ../QUnit/test/log -cp %d -dp %d -m %s -l %s",
            init_port,
            myinfo.dn_port[0],
            qunit_module,
            qunit_level);
        securec_check_ss_c(ret, "", "");
        fprintf(stderr, _("\n%s\n"), buf);
        r = regr_system(buf);
        if (r != 0) {
            fprintf(stderr, _("\n%s: Failed to Run QUnit : exit code was %d\n"), progname, r);
            exit(2);
        }

        if (temp_install && clean) {
            header(_("shutting down postmaster"));
            stop_postmaster(true);
        }
    }

    cleanup_environment();
    return 0;
}