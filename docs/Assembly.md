# Assembly

## File extension .S or .s
Assembly file type extensions must be an uppercase .S (or .sx) if you want them to be preprocessed.

```
    myfile.S    - Pre-process before compilation
    myfile.sx   - Pre-process before compilation
    myfile.s    - No pre-processor
```

If you have lots of compilation errors or linker errors it probably because the file was not pre-processed.

Refer to https://gcc.gnu.org/onlinedocs/gcc/Overall-Options.html
