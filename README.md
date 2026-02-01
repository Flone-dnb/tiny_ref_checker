# tiny ref checker

- Checks that "@ref <some_name>" (in the code) references valid entities (within the same file).

- Usage: `tiny_ref_checker <path_to_src_dir>`
    - recursively checks all ".h", ".hpp", ".c" and ".cpp" files
    - returns 0 if everything is valid, 1 if something is wrong (and prints what's wrong)

