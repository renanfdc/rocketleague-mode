# Acessibilidade visual — guia por perfil

Este projeto é de **inclusão**. O objetivo é **reduzir barreira visual** para uma pessoa com deficiência
visual conseguir acompanhar a partida com conforto — **não** dar vantagem. A bola, os carros e o campo
precisam ficar legíveis; o jogo que os outros jogam não muda.

> Linguagem: falamos de **acesso**, não de superioridade. Evitamos termos capacitistas. O usuário-alvo é
> uma pessoa PCD; as instruções são escritas para ela ou para quem a ajuda.

## Princípios de design visual aplicados

- **Contraste antes de cor.** Distinguir por **forma/contorno/luminância**, não só matiz (parte do público
  não diferencia matizes). Por isso o **High Contrast Nameplates** (muda forma) vale mais que só cor.
- **Evitar o par vermelho-verde.** É o eixo que o daltonismo mais comum (deuteranopia/protanopia, ~8% dos
  homens) não distingue. Cores seguras: **azul, amarelo, ciano, branco**. *(Foi por ignorar isso que a
  textura `high_contrast_ball.png` do mod antigo ficou ruim — ela é verde+vermelho.)*
- **Menos poluição = mais foco.** Desligar efeitos de fundo (Bloom, Light Shafts, Weather, sombras) ajuda a
  isolar a bola e os carros.
- **Não escurecer.** Para baixa visão, **reduzir luminância/contraste global piora** (é por isso que o
  `night_mode` do mod antigo é contraindicado como acessibilidade).
- **Ampliar quando ajuda.** FOV/Distance, UI Scale e a Lupa do Windows aumentam o tamanho aparente dos
  elementos.

## Por perfil

### Daltonismo (deficiência de cor) — o mais comum

1. **No jogo:** Color Blind Mode **On** + High Contrast Nameplates **On**.
2. **Windows:** Filtro de Cor da sua condição (Deuteranopia / Protanopia / Tritanopia) — `Win+Ctrl+C`.
3. **GPU:** aumentar saturação (Digital Vibrance / AMD Custom Color) para separar campo/bola/boost.
4. **Treino (offline):** plugin BakkesMod Color Blind Color Changer para achar o tom ideal e, se quiser,
   textura de bola **azul ou amarela** (nunca vermelha/verde).

### Baixa visão (acuidade reduzida / embaçado)

1. **No jogo:** desligar Bloom/Light Shafts/Weather/Dynamic Shadows/Motion Blur; Render Detail Performance;
   Camera Shake Off; **UI Scale alto**; ajustar **FOV/Distance** para ampliar.
2. **Monitor:** gamma 2.0-2.2, Black Equalizer médio, contraste alto.
3. **GPU:** saturação + nitidez (AMD Radeon Image Sharpening / NVIDIA).
4. **Windows:** Lupa (`Win` `+`) quando precisar ampliar uma região (custo de FPS).
5. **Treino (offline):** textura de bola sólida de **alta saturação** (amarelo/ciano) maximiza a localização
   da bola — ver candidatas em [`../assets/texturas/`](../assets/texturas/).

### Fotofobia / sensibilidade a movimento (vestibular, fotossensível)

1. **No jogo:** Camera Shake Off, Motion Blur Off, desligar Bloom/Light Shafts/Weather.
2. **Monitor:** **baixar** o brilho (aqui escurecer ajuda — perfil oposto ao da baixa visão).
3. **Windows:** Night Light / tema escuro conforme conforto.

## Recursos de acessibilidade além da visão (a explorar)

- **Áudio posicional nativo** do RL (som da bola/boost) é uma pista não-visual relevante para baixa visão
  severa — vale documentar/treinar (ver [ROADMAP.md](../ROADMAP.md)).
- **Outros SO:** macOS tem `Ajustes > Acessibilidade > Filtros de Cor`; SteamOS/Steam Deck têm filtros de
  cor próprios. O mesmo princípio "camada fora do jogo = seguro" se aplica.

## O que NÃO é acessibilidade (para não confundir)

- **Trail/boost cosmético:** item de garagem, não ajuste de visibilidade.
- **Escurecer o cenário (`night_mode`):** reduz contraste; contraindicado para baixa visão.
- **Mod injetado no online:** não é acessibilidade, é risco de conta — ver [riscos-e-limitacoes.md](riscos-e-limitacoes.md).
