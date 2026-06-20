Import("env")

# Remove a flag -Wl,-Map do processo de linkagem para evitar quebra do GCC
# por causa de caracteres especiais na pasta ("7º Periodo")
new_flags = []
for flag in env.get("LINKFLAGS", []):
    if not flag.startswith("-Wl,-Map"):
        new_flags.append(flag)

env.Replace(LINKFLAGS=new_flags)
