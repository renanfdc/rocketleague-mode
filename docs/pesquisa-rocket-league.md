# Pesquisa: estado atual do Rocket League e impacto no projeto

> Pesquisa feita em **jun/2026** (fan-out de buscas web + verificação adversarial da
> afirmação central de risco). Cada bloco traz as fontes. Onde a fonte primária da
> Psyonix/Epic retornou bloqueio (HTTP 403) ao fetch automatizado, o fato está marcado
> como confirmado por fontes secundárias confiáveis que citam o comunicado oficial.

## TL;DR (o que mudou e por que importa)

| Fato | Data | Impacto no projeto |
|---|---|---|
| **Easy Anti-Cheat (EAC) virou obrigatório no online de PC** | 28/04/2026 (Season 22) | O jogo se partiu em dois: **EAC-on** (todo online, sem mods) e **EAC-off** (freeplay/treino/LAN/replay, mods rodam). Mod injetado **só existe offline**. |
| **BakkesMod descontinuado e revivido** com apoio da Psyonix, **só modo non-EAC** | 28/04/2026 | A maior base de mods do RL não roda online. Plugins de acessibilidade existem, mas só offline. |
| **Psyonix passou a absorver features de mod nativamente** | Season 22-23 | Hitbox viz, MMR display, ferramentas de treino e team colors viraram nativos. A margem para mod visual encolheu. |
| **Migração de engine: UE3 → Unreal Engine 6** (não UE5) | Anúncio 24/05/2026; deploy ~2028 | Qualquer mod baseado em injeção na UE3 tende a quebrar de vez na nova engine. Janela longa até lá. |

**Conclusão operacional:** para **online**, o caminho seguro é 100% **fora do processo do jogo**
(config nativa + driver de vídeo + filtros do SO + monitor). Para **treino offline (EAC-off)**, mods
de contraste são usáveis para calibrar a leitura da bola. O mod injetado deste repositório **não tem
caminho seguro para o online** e fica restrito a treino offline.

---

## 1. Easy Anti-Cheat (EAC) no Rocket League

- **Quando:** EAC chegou em **28/04/2026** (Season 22), no PC (Steam e Epic Games Store). Foi o
  primeiro anti-cheat client-side de verdade do jogo. Suporte oficial a Steam Deck e Linux (Proton).
- **Escopo por modo:**
  - **Exigem EAC (EAC-on):** ranqueada, casual online, **partidas privadas online** e torneios in-game.
  - **NÃO exigem EAC (EAC-off):** free play offline, treino customizado, exhibition vs bots, **LAN**,
    replay/edição de vídeo, workshop (Steam).
  - Existe a opção de inicialização **"Play without Easy Anti-Cheat"** (no Steam aparece o diálogo
    "Mods and Limited Online Play"; na Epic adiciona-se `-noEAC`). Com o EAC desligado, **todo o online
    fica bloqueado**.
- **Capacidade técnica de detecção (EAC roda em modo kernel):** detecta as três técnicas que o mod
  deste repo usa:
  1. **Proxy-DLL / DLL injetada** (`winmm.dll`/`version.dll` na pasta do jogo): enumeração de módulos,
     varredura por memória executável sem módulo correspondente, checagem de header PE e assinatura.
  2. **Inline hooks/detours** (ex. `ProcessEvent`): hash periódico das seções `.text` (baseline vs
     re-hash) e inspeção de prólogo de função procurando jumps (`0xE9`, `0xFF 0x25`).
  3. **Escrita externa em memória:** `ObRegisterCallbacks` remove `PROCESS_VM_READ/WRITE` de handles
     externos; detecção de drivers kernel não autorizados.
  > Ponto cego conhecido: cheats kernel-level com driver próprio/ofuscação podem contornar. Isso **não**
  > se aplica a um mod simples, em texto claro, com proxy-DLL + MinHook + memory-write.
- **Política de banimento:** caso a caso, do aviso ao ban permanente; cheating tende a permanente. O ban
  é por **conta Epic** e propaga entre plataformas vinculadas — mas a Epic distingue **ban de produto**
  (só o título) de **ban de conta** (toda a biblioteca) conforme a severidade. Cabe 1 apelação por sanção.

Fontes: rocketleague.com/news/easy-anti-cheat-comes-to-rocket-league-on-pc-today ·
epicgames.com/help (EAC no RL) · dotesports.com/rocket-league/news/rocket-league-easy-anti-cheat ·
thespike.gg/rocket-league/beginner-guides/easy-anti-cheat-in-rocket-league ·
trophi.ai (EAC abril 2026) · safety.epicgames.com/en-US/sanctions-and-appeals ·
literatura técnica de anti-cheat (tateware.com, s4dbrd.github.io, back.engineering).

---

## 2. BakkesMod ainda é viável?

- **Sim, mas só offline.** Em 28/04/2026 o BakkesMod anunciou encerramento; poucos dias depois voltou
  com **apoio oficial da Psyonix**, atualizado para a versão atual do RL **em modo non-EAC apenas**.
  Conta oficial @RocketLeague confirmou suporte continuado, non-EAC only.
- **Não roda online.** Subir COM EAC = online liberado, sem mods. Subir SEM EAC = BakkesMod injeta, mas
  online bloqueado. **Não há bypass:** com EAC ligado o mod nem injeta (`it won't inject into the EAC
  version`), então tentar usar online dá **erro, não ban**.
- **Psyonix tolera/apoia.** Sem histórico de bans por uso padrão (features de QoL/treino/cosmético
  client-side). Risco de ban só com forks anti-EAC, editores de memória, packet injectors ou plugins
  que alterem gameplay (auto-aim/física).
- **Acessibilidade:** existe o plugin **"Color Blind Color Changer"** (autor charlatan, v1.0.2, atualizado
  11/12/2025) e o "Color Changer" (menu F2). Permitem customizar cores além do laranja/azul fixo do modo
  daltônico nativo. Como todo plugin, **só rodam em modo non-EAC (offline)**. Não foi confirmado no GitHub
  se ajustam a cor **da bola** especificamente (confiança média).

Fontes: x.com/BakkesMod/status/2054249895448461494 · x.com/RocketLeague/status/2054253085094994064 ·
dotesports.com/rocket-league/news/bakkesmod-rocket-league-anti-cheat-update ·
pcgamer.com (fim do mod mais popular) · bakkesplugins.com/plugin/623 · rlpeak.com (ban risk).

---

## 3. Recursos NATIVOS de acessibilidade e config visual permitida

Só com o menu oficial (sem mod), o RL já oferece um pacote útil para baixa visão/daltonismo:

- **Color Blind Mode** (Settings > Gameplay, toggle On/Off; desde patch v1.25, 2016): força carros e
  cores de time para laranja/azul de alto contraste (mira deuteranopia). Toggle único, sem seleção de tipo.
- **High Contrast Nameplates** (toggle separado): muda **forma e contorno** das placas do time adversário,
  não só a cor.
- **Ball Cam / Ball Arrow / Ball Cam Indicator:** auxiliares nativos para localizar a bola (a seta aponta a
  direção da bola fora da tela).
- **FOV 60-110, Camera Distance 100-400, Height/Angle/Stiffness:** ampliar elementos aproximando a câmera.
- **Camera Shake Off, Motion Blur Off:** reduzem movimento/tremor na tela.
- **Render Detail (Performance/Custom) + desligar Bloom, Light Shafts, Weather Effects, Dynamic Shadows,
  Ambient Occlusion, Depth of Field:** limpam a poluição visual de fundo. **Este preset também AUMENTA FPS.**
- **UI Scale 0.5-1.0, Safe Zone, Disable Stat Notifications:** HUD maior e menos ruído.
- **HDR Paper White (brilho) e HDR Contrast (gama):** sliders nativos **quando o display é HDR**.
- **Limitação importante:** **não existe slider nativo de brilho/gamma em SDR no PC** (confiança média na
  fonte). Em SDR, brilho só via monitor/GPU. E **não há setting nativo para realçar/recolorir a bola** — a
  alavanca real é Color Blind Mode + desligar efeitos de fundo. Trail/boost são cosméticos, não acessibilidade.

Fontes: epicgames.com/help (Color Blind Mode) · liquipedia.net/rocketleague/Settings ·
liquipedia.net/rocketleague/List_of_player_camera_settings · esports.gg (video settings) ·
hdrgamer.com (RL HDR) · steamcommunity (ausência de slider de brilho SDR).

---

## 4. Camadas FORA do processo (seguras com EAC) — SO / GPU / monitor

Regra-mestra: **cor/saída do DRIVER + do MONITOR + do WINDOWS = seguro** (EAC nem vê);
**overlay/injeção no processo do jogo = risco**.

| Camada | Seguro? | Observação |
|---|---|---|
| **Windows 11 Filtros de Cor** (deuteranopia/protanopia/tritanopia, escala de cinza, inversão) | ✅ Seguro | Win+Ctrl+C. Composição do DWM, por cima de tudo. Primeira escolha para daltonismo. |
| **Windows 11 Lupa/Magnifier** | ✅ Seguro | Amplia região; custo de performance médio-alto (re-amostragem via DWM). |
| **NVIDIA Digital Vibrance** (NVIDIA Control Panel / App) | ✅ Seguro | Saturação na saída do driver. Automatizável por jogo (VibranceGUI / Dynamic Vibrance). |
| **AMD Adrenalin Custom Color + Radeon Image Sharpening** | ✅ Seguro | Equivalente AMD; anti-cheat safe por design. |
| **Intel Graphics Command Center (color)** | ✅ Seguro | Para iGPU; perfil por-jogo fraco (só global). |
| **OSD do monitor** (gamma 2.0-2.2, Black Equalizer, saturação, contraste) | ✅ Seguro | Altera o sinal no hardware, fora do PC. Subutilizado. |
| **NVIDIA Freestyle / GeForce Game Filters** | ⚠️ Zona cinzenta | **Injeta overlay no processo.** Pode não carregar com EAC; há relato de EAC marcar como "software de trapaça" em outros títulos (Hunt, DBD, Insurgency). Trocar por Digital Vibrance. |
| **ReShade** | ⚠️ Arriscado | **Injeta proxy DLL d3d/dxgi.** Só no modo offline sem EAC. Digital Vibrance entrega o mesmo efeito de cor sem injeção. |

> **Pré-requisito crítico:** para os filtros do Windows e a cor do driver pegarem, rode o RL em
> **Borderless/Windowed Fullscreen**, NÃO em fullscreen exclusivo (que assume o display e ignora a camada).

Fontes: nvidia.com/forums (EAC bloqueando Freestyle) · support.microsoft.com (Color Filters) ·
vibrancegui.com · amd.com (Custom Color / RIS) · intel.com (GCC) · forums.bhvr.com (DBD + EAC + GeForce).

---

## 5. O que a comunidade usa (precedente) e acessibilidade da Psyonix

- **Texture mods de bola (AlphaConsole / Custom-Ball-Online):** AlphaConsole standalone foi arquivado em
  **2020**; sobrevive como plugin de BakkesMod. Mesmo antes do EAC, o BakkesMod **já bloqueava bola custom
  em partida pública** (evitar vantagem injusta) — texturas só renderizam em freeplay/custom/treino. O EAC
  selou isso: **textura de bola é ferramenta de TREINO, nunca de ranqueada**.
- **Estratégia da comunidade:** treinar leitura de bola **offline** com contraste maximizado; jogar
  ranqueada com o **nativo** (Color Blind Mode + High Contrast Nameplates) reforçado por filtros de GPU/SO.
- **Psyonix e acessibilidade:** sem roadmap público de acessibilidade em 2025/2026. O teto oficial é o que
  já existe (Color Blind Mode + Nameplates). Comentaristas classificam como básico frente ao padrão de 2026.
  Não há Rocket League 2 anunciado.

Fontes: support.rocketleague.com (Color Blind Mode) · github.com/AlphaConsole (arquivado 2020) ·
github.com/smallest-cock/Custom-Ball-Online · bakkesplugins.com/plugin/623 · prejump.com/training-packs.

---

## 6. Migração de engine (UE6) e horizonte

- Anunciado no RLCS Paris Major (**24/05/2026**): RL migra da **Unreal Engine 3** (desde 2015) **direto
  para a Unreal Engine 6** — pula a UE5. RL é o primeiro showcase público da UE6 da Epic.
- Epic afirma que a **física permanece idêntica** (integridade competitiva), ainda não validado em playtest.
- **Deploy real estimado ~2028** ou depois (builds preview da UE6 a ~2-3 anos). Implicação: a nova engine
  pode quebrar de vez qualquer mod baseado em injeção na UE3; há uma janela longa até lá.

Fontes: tweaktown.com (RL → UE6) · shacknews.com (trailer UE6) · thegamer.com (Paris Major) ·
en.wikipedia.org/wiki/Rocket_League.

---

## Verificação adversarial da afirmação central de risco

**Afirmação testada:** *"Rodar o mod (proxy winmm.dll + MinHook em ProcessEvent + escrita em memória de
UObjects) arrisca ban pelo EAC e por isso não deve ser usado online."*

Três lentes independentes tentaram **refutar** a afirmação. Resultado conciliado:

- **A conclusão "não usar online" está CORRETA** — mas a causa precisa de precisão.
- **Com EAC ativo, o mod é BLOQUEADO (não injeta), não banido na hora** — mesmo comportamento do BakkesMod
  (tentar carregar com EAC dá erro, sem penalidade). O motivo real de "não funciona online" é *"o mod não
  injeta na versão EAC"*, não *"injeta e toma ban"*.
- **O risco REAL de ban surge se o usuário tentar BURLAR o EAC** para forçar o mod injetado numa partida
  online — aí cai na cláusula de software de terceiros que gera ban.
- **Offline (modo EAC-off), o mod é permitido por design e não há risco de ban** — exatamente o cenário que
  a Psyonix desenhou como permitido.

> Síntese honesta: **mod = só offline (EAC-off); online = nativo + camadas externas.** Não há caso concreto
> público de conta banida especificamente por mod **visual client-side** em RL — mas ausência de evidência
> não é evidência de ausência, e um detour/memory-writer desconhecido pode ser classificado diferente de um
> mod whitelistado. Tratar como **risco real ao tentar online; zero ao ficar offline**.

Fontes: ver §1 e §2; verificação detalhada em [riscos-e-limitacoes.md](riscos-e-limitacoes.md).
