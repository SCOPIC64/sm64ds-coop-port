/* ELF does not merge COFF-style $ suffixes. Sort the same contributions
   explicitly, retaining the sentinels and every hosted state variable. */
SECTIONS
{
  .dsstate : ALIGN(4)
  {
    KEEP(*(SORT_BY_NAME(.dsstate$*)))
  }
}
INSERT AFTER .data;
