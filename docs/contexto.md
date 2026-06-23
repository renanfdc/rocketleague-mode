# Contexto do projeto

## O que é

Projeto de **acessibilidade visual** para Rocket League. O objetivo é permitir que uma pessoa
com deficiência visual (baixa visão / daltonismo / cegueira parcial) consiga acompanhar a
partida com mais conforto e legibilidade: ver a bola, os carros, o campo e os elementos
relevantes com contraste e nitidez suficientes.

**O que este projeto NÃO é:**

- Não é cheat, hack ou trapaça.
- Não dá vantagem competitiva.
- Não automatiza jogadas, não mira, não altera física nem rede.
- Não busca superioridade — busca **acesso**. Reduzir barreira visual, não criar desequilíbrio.

Tudo aqui é estritamente **client-side e visual**: o que muda é como o jogo aparece na tela
de quem precisa, não a partida que os outros jogam. Nenhum outro jogador é afetado.

## Para quem é (usuário-alvo)

Uma pessoa PCD com limitação visual — no caso de origem, um amigo do mantenedor que tem
dificuldade de enxergar corretamente elementos importantes da partida (principalmente a bola
contra certos fundos/mapas claros). Ele já havia mexido em ajustes antes para melhorar a
experiência, e este projeto parte desse contexto: recuperar o que foi feito, organizar o que
ficou pendente e finalizar de forma segura.

Premissas sobre o usuário:

- Pode não ser técnico — instalação e uso precisam ser simples e bem documentados.
- Joga em PC (Windows), Rocket League via Steam ou Epic.
- Pode jogar online (casual/ranqueado), treino, partida privada, replay ou offline.
- A **conta não pode correr risco** de banimento por causa de um recurso de acessibilidade.

## Histórico — o que existia antes

O ponto de partida foi um mod chamado **RL Ball Mod** (pasta original `rl-ball-mod`),
desenvolvido como DLL standalone (sem BakkesMod). Resumo do que ele fazia:

- Substituía a **textura da bola** por uma versão de **alto contraste** (amarela, "joaninha"
  preto-e-branco, ou alto-contraste verde) para a bola ficar muito mais fácil de seguir.
- Aplicava um **"céu noturno"** (escurecia a iluminação do mapa) para a bola clara contrastar
  com o fundo.
- Tinha um **instalador** e um **Mod Manager** (GUI em Python) para trocar textura e ajustar
  intensidade sem editar arquivos à mão.

Esse mod está preservado neste repositório em [`../legacy/rl-ball-mod/`](../legacy/rl-ball-mod/),
**apenas como código-fonte arquivado**, pelos motivos técnicos e de segurança documentados em
[riscos-e-limitacoes.md](riscos-e-limitacoes.md) e [pesquisa-rocket-league.md](pesquisa-rocket-league.md).

## Princípio que guia todas as decisões

1. **Acessibilidade, não vantagem.** Toda escolha é avaliada por quanto reduz barreira visual,
   nunca por quanto "ajuda a ganhar".
2. **Segurança da conta primeiro.** Nenhuma solução que coloque a conta em risco de banimento é
   recomendada para uso online. Limitações são documentadas com honestidade, não escondidas.
3. **Simplicidade para quem usa.** A pessoa PCD precisa conseguir configurar sozinha, com guia
   claro e linguagem não capacitista.
4. **Online e offline separados.** O que é seguro em partida online fica claramente separado do
   que só deve ser usado em treino/offline/replay.
