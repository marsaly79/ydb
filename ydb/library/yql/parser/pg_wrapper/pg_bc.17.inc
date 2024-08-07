LLVM_BC(
    postgresql/src/backend/commands/async.c
    postgresql/src/backend/commands/event_trigger.c
    postgresql/src/backend/tsearch/dict_simple.c
    postgresql/src/backend/utils/adt/jsonb_gin.c
    postgresql/src/backend/utils/adt/lockfuncs.c
    postgresql/src/backend/utils/adt/network_spgist.c
    postgresql/src/backend/utils/adt/pg_upgrade_support.c
    postgresql/src/backend/utils/adt/varlena.c
    pg_kernels.17.cpp

    NAME PgFuncs17

    SYMBOLS
    arrow_array_to_text
    arrow_array_to_text_null
    arrow_binary_upgrade_create_empty_extension
    arrow_binary_upgrade_set_missing_value
    arrow_binary_upgrade_set_next_array_pg_type_oid
    arrow_binary_upgrade_set_next_heap_pg_class_oid
    arrow_binary_upgrade_set_next_heap_relfilenode
    arrow_binary_upgrade_set_next_index_pg_class_oid
    arrow_binary_upgrade_set_next_index_relfilenode
    arrow_binary_upgrade_set_next_multirange_array_pg_type_oid
    arrow_binary_upgrade_set_next_multirange_pg_type_oid
    arrow_binary_upgrade_set_next_pg_authid_oid
    arrow_binary_upgrade_set_next_pg_enum_oid
    arrow_binary_upgrade_set_next_pg_tablespace_oid
    arrow_binary_upgrade_set_next_pg_type_oid
    arrow_binary_upgrade_set_next_toast_pg_class_oid
    arrow_binary_upgrade_set_next_toast_relfilenode
    arrow_binary_upgrade_set_record_init_privs
    arrow_btnametextcmp
    arrow_bttext_pattern_cmp
    arrow_bttext_pattern_sortsupport
    arrow_bttextcmp
    arrow_bttextnamecmp
    arrow_bttextsortsupport
    arrow_btvarstrequalimage
    arrow_byteaGetBit
    arrow_byteaGetByte
    arrow_byteaSetBit
    arrow_byteaSetByte
    arrow_bytea_bit_count
    arrow_bytea_sortsupport
    arrow_bytea_string_agg_finalfn
    arrow_bytea_string_agg_transfn
    arrow_bytea_substr
    arrow_bytea_substr_no_len
    arrow_byteacat
    arrow_byteacmp
    arrow_byteaeq
    arrow_byteage
    arrow_byteagt
    arrow_byteain
    arrow_byteale
    arrow_bytealt
    arrow_byteane
    arrow_byteaoctetlen
    arrow_byteaout
    arrow_byteaoverlay
    arrow_byteaoverlay_no_len
    arrow_byteapos
    arrow_bytearecv
    arrow_byteasend
    arrow_dsimple_init
    arrow_dsimple_lexize
    arrow_gin_compare_jsonb
    arrow_gin_consistent_jsonb
    arrow_gin_consistent_jsonb_path
    arrow_gin_extract_jsonb
    arrow_gin_extract_jsonb_path
    arrow_gin_extract_jsonb_query
    arrow_gin_extract_jsonb_query_path
    arrow_gin_triconsistent_jsonb
    arrow_gin_triconsistent_jsonb_path
    arrow_inet_spg_choose
    arrow_inet_spg_config
    arrow_inet_spg_inner_consistent
    arrow_inet_spg_leaf_consistent
    arrow_inet_spg_picksplit
    arrow_name_text
    arrow_nameeqtext
    arrow_namegetext
    arrow_namegttext
    arrow_nameletext
    arrow_namelttext
    arrow_namenetext
    arrow_pg_advisory_lock_int4
    arrow_pg_advisory_lock_int8
    arrow_pg_advisory_lock_shared_int4
    arrow_pg_advisory_lock_shared_int8
    arrow_pg_advisory_unlock_all
    arrow_pg_advisory_unlock_int4
    arrow_pg_advisory_unlock_int8
    arrow_pg_advisory_unlock_shared_int4
    arrow_pg_advisory_unlock_shared_int8
    arrow_pg_advisory_xact_lock_int4
    arrow_pg_advisory_xact_lock_int8
    arrow_pg_advisory_xact_lock_shared_int4
    arrow_pg_advisory_xact_lock_shared_int8
    arrow_pg_blocking_pids
    arrow_pg_column_compression
    arrow_pg_column_size
    arrow_pg_event_trigger_table_rewrite_oid
    arrow_pg_event_trigger_table_rewrite_reason
    arrow_pg_isolation_test_session_is_blocked
    arrow_pg_notification_queue_usage
    arrow_pg_notify
    arrow_pg_safe_snapshot_blocking_pids
    arrow_pg_try_advisory_lock_int4
    arrow_pg_try_advisory_lock_int8
    arrow_pg_try_advisory_lock_shared_int4
    arrow_pg_try_advisory_lock_shared_int8
    arrow_pg_try_advisory_xact_lock_int4
    arrow_pg_try_advisory_xact_lock_int8
    arrow_pg_try_advisory_xact_lock_shared_int4
    arrow_pg_try_advisory_xact_lock_shared_int8
    arrow_replace_text
    arrow_split_part
    arrow_string_agg_combine
    arrow_string_agg_deserialize
    arrow_string_agg_finalfn
    arrow_string_agg_serialize
    arrow_string_agg_transfn
    arrow_text_concat
    arrow_text_concat_ws
    arrow_text_format
    arrow_text_format_nv
    arrow_text_ge
    arrow_text_gt
    arrow_text_larger
    arrow_text_le
    arrow_text_left
    arrow_text_lt
    arrow_text_name
    arrow_text_pattern_ge
    arrow_text_pattern_gt
    arrow_text_pattern_le
    arrow_text_pattern_lt
    arrow_text_reverse
    arrow_text_right
    arrow_text_smaller
    arrow_text_starts_with
    arrow_text_substr
    arrow_text_substr_no_len
    arrow_text_to_array
    arrow_text_to_array_null
    arrow_textcat
    arrow_texteq
    arrow_texteqname
    arrow_textgename
    arrow_textgtname
    arrow_textin
    arrow_textlen
    arrow_textlename
    arrow_textltname
    arrow_textne
    arrow_textnename
    arrow_textoctetlen
    arrow_textout
    arrow_textoverlay
    arrow_textoverlay_no_len
    arrow_textpos
    arrow_textrecv
    arrow_textsend
    arrow_to_hex32
    arrow_to_hex64
    arrow_unicode_is_normalized
    arrow_unicode_normalize_func
    arrow_unistr
    arrow_unknownin
    arrow_unknownout
    arrow_unknownrecv
    arrow_unknownsend
)
