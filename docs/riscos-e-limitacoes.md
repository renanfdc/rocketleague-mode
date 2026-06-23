# Riscos e limitações (honesto, sem esconder)

## 1. Risco de conta (Easy Anti-Cheat) — o ponto central

Desde **28/04/2026** o EAC é **obrigatório para todo jogo online** no PC (ranqueada, casual, partida
privada online, torneios). Isso define toda a estratégia do projeto.

**Verdade técnica precisa (verificada de forma adversarial):**

- **Com o EAC ligado, o mod injetado NÃO injeta — é bloqueado.** Não é "injeta e toma ban na hora". O
  comportamento é o mesmo do BakkesMod: tentar carregar com EAC ativo dá **erro, sem penalidade**.
- **O risco REAL de ban surge ao tentar BURLAR o EAC** para forçar o mod numa partida online. Aí cai na
  cláusula de software de terceiros que gera sanção (caso a caso até **ban permanente**, que **propaga
  entre plataformas vinculadas** à conta Epic).
- **Offline (modo EAC-off), o mod é permitido por design e não há risco de ban** — é o cenário que a
  própria Psyonix desenhou (free play, treino, LAN, replay).
- **Não há caso público** de conta banida especificamente por mod **visual client-side** em RL — mas
  ausência de evidência não é evidência de ausência; um detour/memory-writer desconhecido pode ser
  classificado diferente de um mod whitelistado.

**Conclusão operacional:** mod = **só offline (EAC-off)**; online = **nativo + camadas externas** (a trilha
de [setup-online-seguro.md](setup-online-seguro.md)). As camadas externas (driver, Windows, monitor) **não
injetam** no processo, então o EAC nem as vê — zero risco.

## 2. Por que a abordagem do mod legado não serve para online

O mod em [`../legacy/rl-ball-mod/`](../legacy/rl-ball-mod/) usa exatamente as três técnicas que o EAC ataca:

1. **Proxy `winmm.dll` por DLL search-order hijacking** (DLL na pasta do jogo).
2. **Inline detour com MinHook em `ProcessEvent`** (vtable slot 67 da UE3).
3. **Scan e escrita direta na memória de UObjects** (+ hook de vtable D3D11).

Além disso: o `auto_test` usa **SendInput** para navegar o menu (incluindo o submenu *CREATE ONLINE /
JOIN ONLINE*) — automação de input que o EAC também sinaliza. E há prova documental de que o **online era
objetivo de design**: o código deriva de um projeto chamado **"CustomBallOnline"** (comentário em
`texture_loader.cpp:108`) e o `LEIA-ME.txt` antigo anunciava *"Funciona em Freeplay, Online, Custom Games"*
— afirmação **falsa e perigosa** que este repositório **repudia explicitamente** e não distribui.

## 3. O que NÃO distribuímos (decisões de compliance)

- **Sem binários:** nada de `winmm.dll`, `version.dll`, `RL-Mod-Manager.exe`. DLL/EXE não assinados
  baixados de repo público são risco de **cadeia de suprimento** (qualquer um troca por payload e o usuário
  instala como admin). Só código-fonte.
- **Sem instalador automático:** os `INSTALAR.bat`/`install.bat` antigos copiavam o proxy DLL no jogo vivo
  **sem nenhum gate de EAC-off** — na prática, bypass de anti-cheat empacotado em 1 clique. Removidos.
- **Sem a string "Funciona em Online"** e sem a automação de menu online — repudiadas na doc.

## 4. Limitações técnicas (marcadas [verificado] / [não verificado])

- **[não verificado]** A troca de textura da bola do mod legado **pode nem funcionar offline**: o caminho
  D3D11 recebe o PNG vazio (`dllmain:251`) e o caminho UE3 está com estado `desconhecido`. Não há prova de
  que a bola renderize trocada.
- **[verificado, por leitura de código]** Offsets/slots presos à build Season 22 v2.67 → **quebram a cada
  update** do RL e podem **crashar** o jogo.
- **[verificado]** `high_contrast_ball.png` (o único asset rotulado "acessibilidade" do mod antigo) é
  **verde+vermelho** — o pior par para o daltonismo mais comum. **Anti-acessibilidade.** Mantido só como
  histórico, com aviso; não é recomendado.
- **[verificado]** `night_mode` escurece o cenário → **reduz contraste** → contraindicado para baixa visão.
  Default deve ser **OFF**.
- **[verificado]** Não existe slider de brilho/gamma nativo em **SDR no PC** (confiança média na fonte) — só
  via HDR (quando o display suporta) ou monitor/GPU.

## 5. Limitações de plataforma / política

- **NVIDIA Freestyle / GeForce Game Filters** injetam overlay no processo: podem não carregar com EAC e há
  relato de serem marcados como "software de trapaça" em outros títulos. **Não confiável** — use Digital
  Vibrance (mesmo efeito de cor, sem injeção).
- **ReShade** injeta proxy DLL d3d/dxgi: só no modo offline sem EAC, e mesmo offline o Digital Vibrance
  entrega o mesmo efeito sem injeção.
- **Console (PS/Xbox):** este projeto cobre **PC/Windows**. No console, valem só os recursos nativos do RL
  (Color Blind Mode, HDR) — nada de driver/Windows/mod.
- **Horizonte UE6 (~2028):** a futura migração de engine tende a quebrar de vez qualquer mod baseado em
  injeção na UE3. A trilha nativa + camadas externas é a que sobrevive à mudança.

## 6. Resumo de risco por trilha

| Trilha | Risco de conta | Observação |
|---|---|---|
| Config nativa do RL | **Nenhum** | Oficial. |
| Driver de vídeo (NVIDIA/AMD/Intel) + monitor (OSD) | **Nenhum** | Fora do processo. |
| Filtros/Lupa do Windows | **Nenhum** | Composição do SO. |
| NVIDIA Freestyle | **Baixo-incerto** | Pode flagar; preferir Digital Vibrance. |
| BakkesMod / mod legado **offline (EAC-off)** | **Nenhum** | Permitido por design; não injeta no online. |
| Mod legado / ReShade **no online** | **Alto** | Exige burlar EAC → ban. **Não fazer.** |
