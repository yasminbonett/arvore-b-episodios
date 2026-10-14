# Árvore B dinâmica com nós em arquivos binários

Trabalho Prático I de Estrutura de Dados II (Unesp, Ciência da Computação).

Implementação em C de uma árvore B em que cada nó é guardado em um arquivo
binário separado. Tem busca, inserção e remoção (algoritmos do CLRS), e o grau
mínimo t é escolhido pelo usuário na primeira execução.

O tema é um guia dos episódios do arco do Red John, da série The Mentalist.
A chave é temporada x 100 + episódio (T2E8 = 208), e cada nó guarda título,
duração e nota.

## Arquivos

- `main.c`: código-fonte completo e comentado
- `episodios_red_john.txt`: os 38 episódios usados no programa
- `episodios_pequeno.txt`: 10 episódios, para testes pequenos
- `relatorio.pdf`: relatório com decisões, dinâmica do t e testes

## Como compilar e rodar

    gcc main.c -o arvore_b
    ./arvore_b

No Windows, use `arvore_b.exe`.

Na primeira execução o programa pede o grau mínimo t (inteiro >= 2).
As árvores ficam salvas em `controle.bin` e `no_XXXX.bin` na pasta do programa.
Para recomeçar, apague esses arquivos.
