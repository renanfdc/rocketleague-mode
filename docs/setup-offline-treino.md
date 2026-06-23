# Setup OFFLINE / treino (modo EAC-off)

> Esta trilha é **só para treino offline**: free play, treino customizado, exhibition vs bots, LAN e
> replay — o modo **sem Easy Anti-Cheat (EAC)**. Aqui dá para usar mods de contraste (textura de bola,
> cores customizadas) para **calibrar a leitura da bola** e depois levar a configuração mental para a
> ranqueada usando a trilha nativa de [setup-online-seguro.md](setup-online-seguro.md).

## Regras de ouro

1. **NUNCA tente usar mod no online.** Com o EAC ligado, o mod **nem injeta** (dá erro, não roda). Tentar
   **burlar** o EAC para forçar online é o que gera **ban**. Ver [riscos-e-limitacoes.md](riscos-e-limitacoes.md).
2. **Treino offline ≠ vantagem.** A textura de bola **só aparece para você, offline**. O BakkesMod já
   bloqueia bola customizada em partida pública desde antes do EAC. Isso é treino de **visão**, não trapaça.
3. Use isto para **descobrir** qual contraste/cor você enxerga melhor; a configuração que vale online é a
   nativa (Color Blind Mode + filtros de GPU/SO).

## Como entrar no modo sem EAC

- **Steam:** ao abrir o RL aparece o diálogo "Mods and Limited Online Play" → escolha iniciar **sem EAC**.
- **Epic:** nos três pontinhos do RL na biblioteca, `Launch options`/adicionar `-noEAC`, ou a opção
  "Launch without Easy Anti-Cheat".
- Com o EAC desligado, **todo o online fica bloqueado** (só freeplay/treino/LAN/replay). É esperado.

## Opção A — BakkesMod (recomendado para treino)

O BakkesMod voltou (28/04/2026) com **apoio oficial da Psyonix**, em **modo non-EAC apenas**. É a forma
mais sustentável e tolerada de modificar o visual offline.

- **Plugin "Color Blind Color Changer"** (autor charlatan, v1.0.2): customiza as cores quando o Color Blind
  Mode está ativo — útil para achar o tom que você enxerga melhor antes de fixar no nativo.
- **Texture mods de bola** (plugin AlphaConsole / Custom-Ball-Online): trocam a textura da bola por uma de
  alto contraste **em freeplay/treino**.
- Instale pelo próprio BakkesMod; mantenha-o atualizado (compatibilidade non-EAC pode degradar com updates do jogo).

> Não foi confirmado no GitHub se o plugin ajusta a cor **da bola** especificamente (confiança média). Teste.

## Opção B — Mod legado deste repositório (`legacy/rl-ball-mod/`) — avançado, por sua conta e risco

O mod original deste projeto troca a textura da bola e escurece o céu via **injeção de DLL**. Está
preservado em [`../legacy/rl-ball-mod/`](../legacy/rl-ball-mod/) **apenas como código-fonte** (sem
binários, sem instalador) e **só deve rodar no modo EAC-off**.

Limitações honestas (não escondidas):

- **Funcionamento NÃO verificado.** A troca de textura da bola depende de um caminho de reflexão UE3 com
  estado `desconhecido`; o caminho D3D11 recebe o PNG vazio. Não há prova de que a bola troque sequer offline.
- **Offsets presos a uma build específica** (Season 22 v2.67): **quebram a cada update** do RL e podem
  **crashar** o jogo (deref de ponteiro errado), não degradar suavemente.
- **Custo de frametime / micro-stutter** mesmo offline (hook por frame). Ver [performance.md](performance.md).
- **`night_mode` deve ficar OFF** para acessibilidade: escurecer o cenário **reduz** o contraste e em geral
  **piora** a baixa visão.
- Detalhes técnicos e o que está pendente: [`../legacy/rl-ball-mod/README.md`](../legacy/rl-ball-mod/README.md).

## Texturas de alto contraste

Candidatas **colorblind-safe por construção** (cor única de alta saturação, sem eixo vermelho-verde) estão
em [`../assets/texturas/`](../assets/texturas/) — **não validadas em jogo**, ver o README de lá para os
critérios. As texturas históricas do mod antigo (incluindo uma `high_contrast_ball.png` que é **ruim para
daltonismo**) ficam em [`../legacy/rl-ball-mod/texturas-historicas/`](../legacy/rl-ball-mod/texturas-historicas/)
**apenas como histórico**, com aviso.

## Mapas de treino úteis (offline)

- Workshop (Steam, Play Offline) e training packs por rank (busque por criador/título no próprio jogo —
  códigos antigos mudam). Bons para treinar **leitura e controle de bola** com o contraste maximizado.
