savedcmd_pgtable_walk.mod := printf '%s\n'   pgtable_walk.o | awk '!x[$$0]++ { print("./"$$0) }' > pgtable_walk.mod
