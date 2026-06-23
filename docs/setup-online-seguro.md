# Setup ONLINE seguro (passo a passo)

> Esta é a trilha **principal** do projeto: melhora a visibilidade **sem tocar no jogo**, então é
> **segura com o Easy Anti-Cheat (EAC)** e vale em **ranqueada, casual, partida privada e treino**.
> Nada aqui injeta código no Rocket League — são ajustes do próprio jogo, do driver de vídeo, do
> Windows e do monitor. **Zero risco de banimento.**

Feito para uma pessoa não técnica conseguir seguir sozinha. Faça **na ordem**. Pare quando já estiver
enxergando bem — você não precisa de todos os passos.

---

## PASSO 1 (pré-requisito) — rodar o jogo em janela sem borda

Vários ajustes abaixo (filtros do Windows, cor do driver, lupa) **só funcionam** se o jogo NÃO estiver
em tela cheia exclusiva. Esse é o erro nº 1 ("apliquei e não mudou nada").

1. Abra o Rocket League.
2. `Settings` (Configurações) > `Video`.
3. Em **Window Mode** (Modo de Janela), escolha **Borderless** (ou "Windowed Fullscreen").
4. Aplique.

> Se você só vai usar os ajustes do PASSO 2 (dentro do jogo) e o OSD do monitor (PASSO 5), pode ficar em
> Fullscreen e pular essa exigência. Borderless é necessário a partir do PASSO 3 (Windows) e ajuda no PASSO 4 (driver).

---

## PASSO 2 — ligar a acessibilidade NATIVA do Rocket League

Estes são recursos oficiais do jogo. **Comece sempre por aqui.**

1. `Settings` > `Gameplay`:
   - Ligue **Color Blind Mode** (Modo Daltônico) → força cores de time para **laranja/azul de alto contraste**.
   - Ligue **High Contrast Nameplates** → muda a **forma e o contorno** das placas do time adversário
     (distingue aliado de inimigo sem depender só de cor).
   - Ligue **Ball Cam Indicator** e **Ball Arrow** → ajudam a localizar a bola fora da tela.
2. `Settings` > `Video` > `Advanced` (limpar a poluição visual de fundo — **e ainda ganha FPS**):
   - **Render Detail:** `Performance` (ou `Custom`).
   - **Desligue:** Bloom, Light Shafts, Dynamic Shadows, Motion Blur, Weather Effects, Ambient Occlusion,
     Depth of Field.
   - **Render Quality:** mantenha `High Quality` (nitidez da bola/jogadores).
3. `Settings` > `Camera`:
   - **Camera Shake:** Off (reduz tremor da tela).
   - Ajuste **FOV** e **Distance** ao seu conforto: distância menor deixa os elementos **maiores**.
   - **UI Scale** mais alto deixa o HUD maior.
4. **Se o seu monitor é HDR:** em `Video`, ajuste **HDR Paper White** (brilho geral) e **HDR Contrast**
   (separação claro/escuro). *Em monitor SDR não existe slider de brilho no jogo — use o PASSO 5.*

---

## PASSO 3 — Filtros de Cor do Windows 11 (para daltonismo)

Aplica um filtro por cima de **tudo**, fora do jogo. Seguro com EAC.

1. `Win + Ctrl + C` liga/desliga o filtro de cor rapidamente. Para configurar:
2. `Configurações` > `Acessibilidade` > `Filtros de cor`.
3. Escolha o filtro da sua condição: **Deuteranopia** (vermelho-verde, o mais comum), **Protanopia** ou
   **Tritanopia**. (Escala de cinza e Invertido também existem para casos específicos.)
4. Volte ao jogo (lembre: em **Borderless**, PASSO 1).

> Para **baixa visão** (ampliar), use a **Lupa** do Windows: `Win` + `+` para aproximar, `Win + Esc` para
> sair. Funciona só em Borderless e tem custo de FPS — ver [performance.md](performance.md).

---

## PASSO 4 — Saturação/cor no driver de vídeo (realça a bola e o campo)

Aumentar a vibração de cor faz a bola, o boost e as linhas do campo "saltarem". Ajuste do driver, fora do
processo do jogo, **seguro com EAC**. Use o da **sua** placa:

- **NVIDIA:** NVIDIA App > `Graphics`/`Display` > **Dynamic Vibrance**; ou NVIDIA Control Panel >
  `Ajustar configurações de cor da área de trabalho` > **Vibração Digital** (~55-75%). Para ligar só quando
  o RL abre, use o **VibranceGUI**.
- **AMD:** AMD Software: Adrenalin Edition > `Display` > **Custom Color** (saturação/contraste) e
  **Radeon Image Sharpening** (nitidez). Pode salvar por perfil de jogo.
- **Intel (laptop/iGPU):** Intel Graphics Command Center > `Display` > `Color` (brilho/contraste/saturação;
  só perfil global).

> Lembre do PASSO 1: em fullscreen exclusivo o ajuste de cor do driver pode ser ignorado. Rode em Borderless.

---

## PASSO 5 — Monitor (OSD) — o mais subutilizado e 100% seguro

Mexe no sinal no hardware, fora do PC inteiro. Use os botões físicos do monitor:

- **Gamma 2.0-2.2** (2.0 levanta as sombras → enxerga melhor áreas escuras).
- **Black Equalizer / Shadow Boost** em nível médio (~10-15) → realça detalhe em sombra.
- **Saturação/Contraste** somam ao Digital Vibrance do PASSO 4.
- Prefira preset **Custom/User/Standard** (evite `Vivid`/`Gaming` travados).
- Use **um** caminho de luz azul (OSD Low Blue Light **ou** Windows Night Light, não os dois).

---

## Ordem recomendada por necessidade

- **Daltonismo (vermelho-verde):** PASSO 2 (Color Blind Mode + Nameplates) → PASSO 3 (Filtro Deuteranopia) →
  PASSO 4 (saturação). Geralmente resolve.
- **Baixa visão (enxerga pouco/embaçado):** PASSO 2 (FOV/Distance + UI Scale + desligar efeitos) → PASSO 5
  (gamma/contraste do monitor) → PASSO 4 (saturação) → Lupa do Windows se precisar ampliar.
- **Fotofobia / sensibilidade a movimento:** PASSO 2 (Camera Shake Off, Motion Blur Off, desligar Bloom/Light
  Shafts/Weather) → PASSO 5 (baixar brilho do monitor) → Windows Night Light.

Guia detalhado por perfil em [acessibilidade-visual.md](acessibilidade-visual.md). Custo de FPS de cada
opção em [performance.md](performance.md).

---

## Se "não funcionou"

| Sintoma | Causa provável | Correção |
|---|---|---|
| Apliquei filtro do Windows/cor do driver e nada mudou | Jogo em **fullscreen exclusivo** | Volte ao **PASSO 1** (Borderless). |
| Não tem slider de brilho no jogo | Monitor **SDR** no PC não tem (não é bug) | Use o **monitor (PASSO 5)** ou a GPU. |
| A lupa derrubou meu FPS | Custo de re-amostragem do DWM | Use só quando precisar; ver [performance.md](performance.md). |
| Quero trocar a textura da bola por uma de alto contraste | Isso **não é nativo** e **não vale online** | Só em treino offline → [setup-offline-treino.md](setup-offline-treino.md). |
