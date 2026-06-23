# Legado: RL Ball Mod (arquivado)

> ⚠️ **ARQUIVADO — não use em partidas online.** Este é o mod original, preservado como **código-fonte**
> de referência. A abordagem dele (injeção de DLL + hooks na memória do jogo) é **incompatível com o Easy
> Anti-Cheat** e só pode rodar no **modo EAC-off (offline/treino)**. **Binários, instalador e a automação
> de menu online foram removidos de propósito** (ver [../../docs/riscos-e-limitacoes.md](../../docs/riscos-e-limitacoes.md)).
> Tentar burlar o EAC para usar online pode **banir a conta**.

## O que ele fazia

Recurso de acessibilidade visual, 100% client-side, sem BakkesMod:

- **Textura da bola de alto contraste** — troca a textura da bola por um PNG.
- **Céu/iluminação "noturna"** — escurece a luz do mapa. *Contraindicado para acessibilidade: escurecer
  reduz contraste. `night_mode` deve ficar OFF.*
- **Mod Manager (GUI)** — `mod-manager/mod_manager.py` (Tkinter), editor de `config.json` com hot-reload.

## Como era construído (resumo técnico)

| Camada | Arquivo(s) | Técnica |
|---|---|---|
| Carregamento | proxy `winmm.dll` (`src/proxy.cpp`, `src/proxy.def`) | **DLL search-order hijack**: DLL falsa na pasta do jogo, reencaminha as 180 exports do winmm real. |
| Hooking | `src/hooks/`, `lib/MinHook/` | **MinHook** instala detours em `ProcessEvent` (UE3, vtable slot 67). |
| Reflexão UE3 | `src/ue3/`, `src/util/pattern_scan.cpp` | Pattern scan acha `GNames`/`GObjects`; chama `UFunction`s via `ProcessEvent`. |
| Render | `src/d3d11/` | Hook D3D11 (Present + PSSetShaderResources via vtable) + escrita direta em memória (luz). |
| Textura | `src/texture/`, `lib/stb_image.h` | Carrega PNG e injeta no material da bola. |

## Estado (verificado x não verificado)

- **[não verificado]** A troca de textura da bola **pode nem funcionar offline**: `dllmain.cpp:251` chama
  `TexHook::Init("", ...)` com PNG **vazio** (caminho D3D11 só faz o céu); a troca de PNG depende do caminho
  UE3 (`ball_hooks.cpp`) com estado `desconhecido`. Não há prova de que a bola renderize trocada.
- **[verificado por leitura]** Offsets/slots presos à build **Season 22 v2.67** → quebram a cada update e
  podem **crashar** o jogo. Nomes de `UFunction` hard-coded (`StartBallFadeIn`, `SetTextureParameterValue`,
  `EventPreLoadMap`) são o ponto provável de "parou de funcionar pós-patch".
- **[verificado]** `auto_test` usava **SendInput** para navegar o menu (incluindo *CREATE/JOIN ONLINE*) —
  capacidade que foi repudiada; manter `auto_test=false`.
- **[verificado]** Código deriva de um projeto **"CustomBallOnline"** (comentário em `texture_loader.cpp:108`)
  e o `LEIA-ME.txt` antigo dizia "Funciona em Online" — **falso e perigoso**; por isso ele **não foi
  incluído** neste repositório (a afirmação está documentada e repudiada nos docs, não reproduzida aqui).
- Pendências de performance (log mode ligado, stutter cíclico, vazamento de VRAM, render corrompido):
  [../../docs/performance.md](../../docs/performance.md).

## O que está aqui (e o que não está)

```
src/            código C++ do mod
lib/            dependências vendorizadas (MinHook, stb_image, json.hpp) — terceiros
build.bat       script de build (g++/MinGW) — assume o WinLibs do WinGet num caminho fixo, ajuste antes de usar
config/         config.json original (referência)
mod-manager/    mod_manager.py (editor de config; paths de instalação sanitizados)
texturas-historicas/  PNGs antigos — NÃO acessíveis, ver README de lá
```

**Não incluído de propósito:** `winmm.dll`/`version.dll`/`RL-Mod-Manager.exe` (binários não assinados),
`INSTALAR.bat`/`install.bat`/`uninstall.bat` (instalador sem gate de EAC-off), o `LEIA-ME.txt` antigo
(anunciava "Funciona em Online" + instruções de instalar a DLL), zips e ~70 screenshots de dev.

## Como buildar (só estudo/offline, por sua conta e risco)

Requer MinGW (WinLibs UCRT). Edite o caminho do compilador no `build.bat` (hoje assume o WinLibs do WinGet)
ou resolva `g++` do PATH; ele compila o `winmm.dll`. **Não há garantia** de funcionar com a build atual do
RL e **não deve rodar com EAC ativo**.
