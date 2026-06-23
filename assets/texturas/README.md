# Texturas de bola (alto contraste) — uso OFFLINE / treino

> Trocar a textura da bola **só vale offline (EAC-off)** e **só aparece para você**. Não vale online e não
> é vantagem (o BakkesMod já bloqueia bola custom em partida pública). Ver
> [../../docs/setup-offline-treino.md](../../docs/setup-offline-treino.md).

## Candidatas incluídas

| Arquivo | Cor | Status |
|---|---|---|
| `bola-amarelo-solido.png` | Amarelo âmbar (#FFD400) | **colorblind-safe por construção** · `[não validado em jogo]` |
| `bola-ciano-solido.png` | Ciano (#00D2E6) | **colorblind-safe por construção** · `[não validado em jogo]` |

São **preenchimentos sólidos** de cor única de alta saturação, 2048×2048. Por quê assim:

- **Colorblind-safe por construção:** cor única não exige discriminação no eixo **vermelho-verde** (o
  daltonismo mais comum). Amarelo e ciano são distinguíveis em deuteranopia, protanopia e tritanopia.
- **Independente do mapeamento UV:** um preenchimento sólido cobre a bola inteira sem depender de como a
  textura é esticada na esfera — o que evita o risco de uma arte detalhada ficar errada num UV desconhecido.
- **Maximiza a localização da bola:** para quem tem dificuldade de **ver** a bola, achá-la rápido importa
  mais do que ler o giro. Trade-off: bola sólida dá menos pista de rotação.

## Critérios para uma boa textura de acessibilidade (se for criar outra)

1. **Não usar o par vermelho-verde.** Prefira azul, amarelo, ciano ou branco.
2. **Alta luminância + alta saturação** contra os campos (que costumam ser claros/esverdeados).
3. Se quiser pista de giro, **borda/contorno branco grosso** + núcleo de cor única (mantém contraste e
   ajuda forma), validando em simulador de daltonismo (deuteranopia/protanopia/tritanopia).
4. **Validar em jogo** antes de chamar de "recomendada" — nenhuma destas foi testada renderizada na bola.

## Aviso sobre as texturas históricas

As texturas do mod antigo estão em
[`../../legacy/rl-ball-mod/texturas-historicas/`](../../legacy/rl-ball-mod/texturas-historicas/) **apenas
como histórico**. Em especial, `high_contrast_ball.png` é **verde+vermelho** — ruim para o daltonismo mais
comum. **Não use como referência de acessibilidade.**
