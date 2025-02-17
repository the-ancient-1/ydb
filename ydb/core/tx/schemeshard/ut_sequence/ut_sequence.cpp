#include <ydb/core/tx/datashard/datashard_ut_common_kqp.h>
#include <ydb/core/tx/schemeshard/ut_helpers/helpers.h>

using namespace NKikimr::NSchemeShard;
using namespace NKikimr;
using namespace NKikimrSchemeOp;
using namespace NSchemeShardUT_Private;

Y_UNIT_TEST_SUITE(TSequence) {

    Y_UNIT_TEST(CreateSequence) {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);
        ui64 txId = 100;

        runtime.SetLogPriority(NKikimrServices::FLAT_TX_SCHEMESHARD, NActors::NLog::PRI_TRACE);
        runtime.SetLogPriority(NKikimrServices::SEQUENCESHARD, NActors::NLog::PRI_TRACE);

        TestCreateSequence(runtime, ++txId, "/MyRoot", R"(
            Name: "seq"
        )");
        env.TestWaitNotification(runtime, txId);

        TestLs(runtime, "/MyRoot/seq", false, NLs::PathExist);
    }

    Y_UNIT_TEST(CreateSequenceParallel) {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);
        ui64 txId = 100;

        runtime.SetLogPriority(NKikimrServices::FLAT_TX_SCHEMESHARD, NActors::NLog::PRI_TRACE);
        runtime.SetLogPriority(NKikimrServices::SEQUENCESHARD, NActors::NLog::PRI_TRACE);

        // Make two passes, so we test parallel creation both when
        // sequenceshard doesn't exist yet, and when it exists already
        for (int j = 0; j < 2; ++j) {
            for (int i = 4*j + 1; i <= 4*j + 4; ++i) {
                TestCreateSequence(runtime, ++txId, "/MyRoot", Sprintf(R"(
                    Name: "seq%d"
                )", i));
            }
            env.TestWaitNotification(runtime, {txId-3, txId-2, txId-1, txId});

            for (int i = 4*j + 1; i <= 4*j + 4; ++i) {
                TestLs(runtime, Sprintf("/MyRoot/seq%d", i), false, NLs::PathExist);
            }
        }
    }

    Y_UNIT_TEST(CreateSequenceSequential) {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);
        ui64 txId = 100;

        runtime.SetLogPriority(NKikimrServices::FLAT_TX_SCHEMESHARD, NActors::NLog::PRI_TRACE);
        runtime.SetLogPriority(NKikimrServices::SEQUENCESHARD, NActors::NLog::PRI_TRACE);

        for (int i = 1; i <= 4; ++i) {
            TestCreateSequence(runtime, ++txId, "/MyRoot", Sprintf(R"(
                Name: "seq%d"
            )", i));
            env.TestWaitNotification(runtime, txId);

            TestLs(runtime, Sprintf("/MyRoot/seq%d", i), false, NLs::PathExist);
        }
    }

    Y_UNIT_TEST(CreateDropRecreate) {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);
        ui64 txId = 100;

        runtime.SetLogPriority(NKikimrServices::FLAT_TX_SCHEMESHARD, NActors::NLog::PRI_TRACE);
        runtime.SetLogPriority(NKikimrServices::SEQUENCESHARD, NActors::NLog::PRI_TRACE);

        TestCreateSequence(runtime, ++txId, "/MyRoot", R"(
            Name: "seq"
        )");
        env.TestWaitNotification(runtime, txId);

        TestLs(runtime, "/MyRoot/seq", false, NLs::PathExist);

        TestDropSequence(runtime, ++txId, "/MyRoot", "seq");
        env.TestWaitNotification(runtime, txId);

        TestLs(runtime, "/MyRoot/seq", false, NLs::PathNotExist);

        TestCreateSequence(runtime, ++txId, "/MyRoot", R"(
            Name: "seq"
        )");
        env.TestWaitNotification(runtime, txId);

        TestLs(runtime, "/MyRoot/seq", false, NLs::PathExist);
    }

    Y_UNIT_TEST(CreateSequenceInsideSequenceNotAllowed) {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);
        ui64 txId = 100;

        runtime.SetLogPriority(NKikimrServices::FLAT_TX_SCHEMESHARD, NActors::NLog::PRI_TRACE);
        runtime.SetLogPriority(NKikimrServices::SEQUENCESHARD, NActors::NLog::PRI_TRACE);

        TestCreateSequence(runtime, ++txId, "/MyRoot", R"(
            Name: "seq"
        )");
        env.TestWaitNotification(runtime, txId);

        TestCreateSequence(runtime, ++txId, "/MyRoot/seq", R"(
            Name: "seq"
        )", {NKikimrScheme::StatusPathIsNotDirectory});
    }

    Y_UNIT_TEST(CreateSequenceInsideTableThenDropSequence) {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);
        ui64 txId = 100;

        runtime.SetLogPriority(NKikimrServices::FLAT_TX_SCHEMESHARD, NActors::NLog::PRI_TRACE);
        runtime.SetLogPriority(NKikimrServices::SEQUENCESHARD, NActors::NLog::PRI_TRACE);

        TestCreateTable(runtime, ++txId, "/MyRoot", R"(
            Name: "Table"
            Columns { Name: "key"   Type: "Uint64" }
            Columns { Name: "value" Type: "Utf8" }
            KeyColumnNames: ["key"]
        )");
        env.TestWaitNotification(runtime, txId);

        TestCreateSequence(runtime, ++txId, "/MyRoot/Table", R"(
            Name: "seq"
        )");
        env.TestWaitNotification(runtime, txId);

        TestLs(runtime, "/MyRoot/Table/seq", TDescribeOptionsBuilder().SetShowPrivateTable(true), NLs::PathExist);

        TestDropSequence(runtime, ++txId, "/MyRoot/Table", "seq");
        env.TestWaitNotification(runtime, txId);

        TestLs(runtime, "/MyRoot/Table/seq", TDescribeOptionsBuilder().SetShowPrivateTable(true), NLs::PathNotExist);
    }

    Y_UNIT_TEST(CreateSequenceInsideTableThenDropTable) {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);
        ui64 txId = 100;

        runtime.SetLogPriority(NKikimrServices::FLAT_TX_SCHEMESHARD, NActors::NLog::PRI_TRACE);
        runtime.SetLogPriority(NKikimrServices::SEQUENCESHARD, NActors::NLog::PRI_TRACE);

        TestCreateTable(runtime, ++txId, "/MyRoot", R"(
            Name: "Table"
            Columns { Name: "key"   Type: "Uint64" }
            Columns { Name: "value" Type: "Utf8" }
            KeyColumnNames: ["key"]
        )");
        env.TestWaitNotification(runtime, txId);

        TestCreateSequence(runtime, ++txId, "/MyRoot/Table", R"(
            Name: "seq"
        )");
        env.TestWaitNotification(runtime, txId);

        TestLs(runtime, "/MyRoot/Table/seq", TDescribeOptionsBuilder().SetShowPrivateTable(true), NLs::PathExist);

        TestDropTable(runtime, ++txId, "/MyRoot", "Table");
        env.TestWaitNotification(runtime, txId);

        TestLs(runtime, "/MyRoot/Table", false, NLs::PathNotExist);
    }

    Y_UNIT_TEST(CreateSequenceInsideIndexTableNotAllowed) {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);
        ui64 txId = 100;

        runtime.SetLogPriority(NKikimrServices::FLAT_TX_SCHEMESHARD, NActors::NLog::PRI_TRACE);
        runtime.SetLogPriority(NKikimrServices::SEQUENCESHARD, NActors::NLog::PRI_TRACE);

        TestCreateIndexedTable(runtime, ++txId, "/MyRoot", R"(
            TableDescription {
                Name: "Table"
                Columns { Name: "key"   Type: "Uint64" }
                Columns { Name: "value" Type: "Utf8" }
                KeyColumnNames: ["key"]
            }
            IndexDescription {
                Name: "ValueIndex"
                KeyColumnNames: ["value"]
            }
        )");
        env.TestWaitNotification(runtime, txId);

        TestCreateSequence(runtime, ++txId, "/MyRoot/Table/ValueIndex/indexImplTable", R"(
            Name: "seq"
        )", {NKikimrScheme::StatusNameConflict});
    }

    Y_UNIT_TEST(CreateSequencesWithIndexedTable) {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);
        ui64 txId = 100;

        runtime.SetLogPriority(NKikimrServices::FLAT_TX_SCHEMESHARD, NActors::NLog::PRI_TRACE);
        runtime.SetLogPriority(NKikimrServices::SEQUENCESHARD, NActors::NLog::PRI_TRACE);

        TestCreateIndexedTable(runtime, ++txId, "/MyRoot", R"(
            TableDescription {
                Name: "Table"
                Columns { Name: "key"   Type: "Uint64" }
                Columns { Name: "value" Type: "Utf8" }
                KeyColumnNames: ["key"]
            }
            IndexDescription {
                Name: "ValueIndex"
                KeyColumnNames: ["value"]
            }
            SequenceDescription {
                Name: "seq1"
            }
            SequenceDescription {
                Name: "seq2"
            }
        )");
        env.TestWaitNotification(runtime, txId);

        TestLs(runtime, "/MyRoot/Table/seq1", TDescribeOptionsBuilder().SetShowPrivateTable(true), NLs::PathExist);
        TestLs(runtime, "/MyRoot/Table/seq2", TDescribeOptionsBuilder().SetShowPrivateTable(true), NLs::PathExist);

        TestDropTable(runtime, ++txId, "/MyRoot", "Table");
        env.TestWaitNotification(runtime, txId);

        TestLs(runtime, "/MyRoot/Table", false, NLs::PathNotExist);
    }

    Y_UNIT_TEST(CreateTableWithDefaultFromSequence) {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);
        ui64 txId = 100;

        runtime.SetLogPriority(NKikimrServices::FLAT_TX_SCHEMESHARD, NActors::NLog::PRI_TRACE);
        runtime.SetLogPriority(NKikimrServices::SEQUENCESHARD, NActors::NLog::PRI_TRACE);

        // Cannot use default from sequence that doesn't exist
        TestCreateIndexedTable(runtime, ++txId, "/MyRoot", R"(
            TableDescription {
                Name: "Table"
                Columns { Name: "key"   Type: "Uint64" DefaultFromSequence: "/MyRoot/myseq" }
                Columns { Name: "value" Type: "Utf8" }
                KeyColumnNames: ["key"]
            }
        )", {NKikimrScheme::StatusInvalidParameter});

        // Cannot use default from sequence that doesn't match local sequences
        TestCreateIndexedTable(runtime, ++txId, "/MyRoot", R"(
            TableDescription {
                Name: "Table"
                Columns { Name: "key"   Type: "Uint64" DefaultFromSequence: "myseq" }
                Columns { Name: "value" Type: "Utf8" }
                KeyColumnNames: ["key"]
            }
            SequenceDescription {
                Name: "someseq"
            }
        )", {NKikimrScheme::StatusInvalidParameter});

        // Cannot use default from sequence for a non-key column
        TestCreateIndexedTable(runtime, ++txId, "/MyRoot", R"(
            TableDescription {
                Name: "Table"
                Columns { Name: "key"   Type: "Uint64" }
                Columns { Name: "value" Type: "Uint64" DefaultFromSequence: "myseq" }
                KeyColumnNames: ["key"]
            }
            SequenceDescription {
                Name: "myseq"
            }
        )", {NKikimrScheme::StatusInvalidParameter});

        TestCreateIndexedTable(runtime, ++txId, "/MyRoot", R"(
            TableDescription {
                Name: "Table"
                Columns { Name: "key"   Type: "Uint64" DefaultFromSequence: "myseq" }
                Columns { Name: "value" Type: "Utf8" }
                KeyColumnNames: ["key"]
            }
            SequenceDescription {
                Name: "myseq"
            }
        )");
        env.TestWaitNotification(runtime, txId);

        TestLs(runtime, "/MyRoot/Table/myseq", TDescribeOptionsBuilder().SetShowPrivateTable(true), NLs::PathExist);

        // Cannot drop sequence used by a column
        TestDropSequence(runtime, ++txId, "/MyRoot/Table", "myseq",
            {NKikimrScheme::StatusNameConflict});

        TestDropTable(runtime, ++txId, "/MyRoot", "Table");
        env.TestWaitNotification(runtime, txId);

        TestLs(runtime, "/MyRoot/Table", false, NLs::PathNotExist);
    }

    Y_UNIT_TEST(CreateTableWithDefaultFromSequenceAndIndex) {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);
        ui64 txId = 100;

        runtime.SetLogPriority(NKikimrServices::FLAT_TX_SCHEMESHARD, NActors::NLog::PRI_TRACE);
        runtime.SetLogPriority(NKikimrServices::SEQUENCESHARD, NActors::NLog::PRI_TRACE);

        TestCreateIndexedTable(runtime, ++txId, "/MyRoot", R"(
            TableDescription {
                Name: "Table"
                Columns { Name: "key"   Type: "Uint64" DefaultFromSequence: "myseq" }
                Columns { Name: "value" Type: "Utf8" }
                KeyColumnNames: ["key"]
            }
            IndexDescription {
                Name: "ValueIndex"
                KeyColumnNames: ["value"]
            }
            SequenceDescription {
                Name: "myseq"
            }
        )");
        env.TestWaitNotification(runtime, txId);

        TestLs(runtime, "/MyRoot/Table/ValueIndex", TDescribeOptionsBuilder().SetShowPrivateTable(true), NLs::PathExist);
        TestLs(runtime, "/MyRoot/Table/myseq", TDescribeOptionsBuilder().SetShowPrivateTable(true), NLs::PathExist);

        TestDropTable(runtime, ++txId, "/MyRoot", "Table");
        env.TestWaitNotification(runtime, txId);

        TestLs(runtime, "/MyRoot/Table", false, NLs::PathNotExist);
    }

    Y_UNIT_TEST(CopyTableWithSequence) {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);
        ui64 txId = 100;

        runtime.SetLogPriority(NKikimrServices::FLAT_TX_SCHEMESHARD, NActors::NLog::PRI_TRACE);
        runtime.SetLogPriority(NKikimrServices::SEQUENCESHARD, NActors::NLog::PRI_TRACE);

        TestCreateIndexedTable(runtime, ++txId, "/MyRoot", R"(
            TableDescription {
                Name: "Table"
                Columns { Name: "key"   Type: "Uint64" DefaultFromSequence: "myseq" }
                Columns { Name: "value" Type: "Utf8" }
                KeyColumnNames: ["key"]
            }
            IndexDescription {
                Name: "ValueIndex"
                KeyColumnNames: ["value"]
            }
            SequenceDescription {
                Name: "myseq"
            }
        )");

        env.TestWaitNotification(runtime, txId);

        TestCopyTable(runtime, ++txId, "/MyRoot", "copy", "/MyRoot/Table");
        env.TestWaitNotification(runtime, txId);

        TestLs(runtime, "/MyRoot/copy/myseq", TDescribeOptionsBuilder().SetShowPrivateTable(true), NLs::PathExist);
        env.TestWaitNotification(runtime, txId);
    }

    Y_UNIT_TEST(AlterSequence) {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);
        ui64 txId = 100;

        runtime.SetLogPriority(NKikimrServices::FLAT_TX_SCHEMESHARD, NActors::NLog::PRI_TRACE);
        runtime.SetLogPriority(NKikimrServices::SEQUENCESHARD, NActors::NLog::PRI_TRACE);

        TestCreateSequence(runtime, ++txId, "/MyRoot", R"(
            Name: "seq"
        )");
        env.TestWaitNotification(runtime, txId);

        TestDescribeResult(DescribePath(runtime, "/MyRoot/seq", true),
            {
                NLs::SequenceIncrement(1),
                NLs::SequenceMinValue(1),
                NLs::SequenceCache(1),
                NLs::SequenceStartValue(1),
                NLs::SequenceCycle(false)
            }
        );

        auto value = DoNextVal(runtime, "/MyRoot/seq");
        UNIT_ASSERT_VALUES_EQUAL(value, 1);

        value = DoNextVal(runtime, "/MyRoot/seq");
        UNIT_ASSERT_VALUES_EQUAL(value, 2);

        TestAlterSequence(runtime, ++txId, "/MyRoot", R"(
            Name: "seq"
            Increment: 2
            MaxValue: 5
            MinValue: 2
            Cache: 1
            StartValue: 2
            Cycle: true
        )");
        env.TestWaitNotification(runtime, txId);

        TestDescribeResult(DescribePath(runtime, "/MyRoot/seq", true),
                {
                    NLs::SequenceIncrement(2),
                    NLs::SequenceMaxValue(5),
                    NLs::SequenceMinValue(2),
                    NLs::SequenceCache(1),
                    NLs::SequenceStartValue(2),
                    NLs::SequenceCycle(true)
                }
        );

        value = DoNextVal(runtime, "/MyRoot/seq");
        UNIT_ASSERT_VALUES_EQUAL(value, 3);

        value = DoNextVal(runtime, "/MyRoot/seq");
        UNIT_ASSERT_VALUES_EQUAL(value, 5);

        value = DoNextVal(runtime, "/MyRoot/seq");
        UNIT_ASSERT_VALUES_EQUAL(value, 2);

        TestAlterSequence(runtime, ++txId, "/MyRoot", R"(
            Name: "seq"
            Cycle: false
            MaxValue: 4
        )");
        env.TestWaitNotification(runtime, txId);

        value = DoNextVal(runtime, "/MyRoot/seq");
        UNIT_ASSERT_VALUES_EQUAL(value, 4);

        DoNextVal(runtime, "/MyRoot/seq", Ydb::StatusIds::SCHEME_ERROR);

        TestAlterSequence(runtime, ++txId, "/MyRoot", R"(
            Name: "seq"
            Cycle: false
            MinValue: 7
        )", {NKikimrScheme::StatusInvalidParameter});

        TestAlterSequence(runtime, ++txId, "/MyRoot", R"(
            Name: "seq"
            Cycle: false
            MinValue: 3
        )", {NKikimrScheme::StatusInvalidParameter});

        TestAlterSequence(runtime, ++txId, "/MyRoot", R"(
            Name: "seq"
            Cycle: false
            MinValue: 3
            StartValue: 3
        )");
        env.TestWaitNotification(runtime, txId);

        TestAlterSequence(runtime, ++txId, "/MyRoot", R"(
            Name: "seq"
            Cycle: false
            MinValue: 3
            MaxValue: 2
        )", {NKikimrScheme::StatusInvalidParameter});
    }

} // Y_UNIT_TEST_SUITE(TSequence)
