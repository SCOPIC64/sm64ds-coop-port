/* Engine globals for gate1_probe.cpp. Plain C, and deliberately a separate TU:
   see the note in gate1_probe.cpp about data_020a4b68 being declared `int *` by
   fault_probe.h and `int []` by the walker. Both spellings resolve to this one
   symbol at link time, which is exactly what happens in the shipped binary. */

int data_020a4b68[4];        /* the walk's published cursor */
int data_020a4b6c[4];
void *data_0209f5bc;         /* fader watch */
signed char data_0209f2f8;   /* current level, printed into the dump */

/* The scene-tree successor. The rig only walks the flat list, so this is never
   reached; it exists to satisfy the walker TU's reference. */
void *func_0203b394(void *node)
{
    (void)node;
    return 0;
}
