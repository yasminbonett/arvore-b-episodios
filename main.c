/* =====================================================================
 * Trabalho Prático - Árvore B (ED II)
 * Tema: Guia do Arco do Red John (O Mentalista)
 * Nome: Yasmin de Oliveira Bonet
 * ===================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//Constantes
#define TAM_TITULO 64   //Tamanho máx do título
#define TAM_NOME   32   //tamanho máximo do nome de um arquivo de nó

//Estrutura de um nó da árvore B (definindo agora para evitar warnings)
typedef struct BTreeNode BTreeNode;

BTreeNode *createNode(int T, bool leaf);
void       freeNode(BTreeNode *node, int T);

//Dados de cada episódio (chave não está aqui, está no nó)
typedef struct {
    char  titulo[TAM_TITULO];
    int   duracao;   //min
    float nota;      //Nota IMDb
} Episodio;

//Nó da árvore B (campos extras: dados e nomeArquivo)
struct BTreeNode {
    int        n;                     //Qnt de chaves no nó
    int       *keys;                  //Chaves (n° do episódio)
    Episodio  *dados;                 //dados[i] e o episodio da chave keys[i]
    char     **children;              //nomes dos arquivos dos filhos
    bool       leaf;                  //true = folha
    char       nomeArquivo[TAM_NOME]; //arquivo onde o nó está salvo
};

//Libera a memória de um nó (pode ser chamado com NULL)
void freeNode(BTreeNode *node, int T) {
    if (node == NULL) return;
    if (node->children != NULL) {
        for (int i = 0; i < 2 * T; i++) {
            free(node->children[i]);   //free(NULL) nao faz nada, ent não quebra
        }
    }
    free(node->children);
    free(node->dados);
    free(node->keys);
    free(node);
}

//Cria um nó vazio com espaço para 2T-1 chaves e 2T filhos
BTreeNode *createNode(int T, bool leaf) {
    BTreeNode *node = (BTreeNode *) calloc(1, sizeof(BTreeNode));
    if (node == NULL) return NULL;

    node->n = 0;
    node->leaf = leaf;
    node->nomeArquivo[0] = '\0';

    node->keys     = (int *)      calloc(2 * T - 1, sizeof(int));
    node->dados    = (Episodio *) calloc(2 * T - 1, sizeof(Episodio));
    node->children = (char **)    calloc(2 * T,     sizeof(char *));

    if (node->keys == NULL || node->dados == NULL || node->children == NULL) {
        freeNode(node, T);
        return NULL;
    }

    //Todo filho comeca com nome vazio, que quer dizer "sem filho"
    for (int i = 0; i < 2 * T; i++) {
        node->children[i] = (char *) calloc(TAM_NOME, sizeof(char));
        if (node->children[i] == NULL) {
            freeNode(node, T);
            return NULL;
        }
    }
    return node;
}

//Controle da árvore
typedef struct {
    int  T;                  //Grau mínimo
    int  proximoNumero;      //Próximo número livre pra nome
    char raiz[TAM_NOME];     //Nome do arquivo do nó raiz
} Controle;

#define ARQ_CONTROLE "controle.bin"

//Gera o nome do arquivo de um nó e incrementa o próximo número livre
void gerarNomeArquivo(Controle *ctrl, char destino[TAM_NOME]) {
    snprintf(destino, TAM_NOME, "no_%04d.bin", ctrl->proximoNumero);
    ctrl->proximoNumero++;
}

//Salva o controle da árvore no arquivo controle.bin (retorna true se deu certo)
bool salvarControle(const Controle *ctrl) {
    FILE *f = fopen(ARQ_CONTROLE, "wb");
    if (f == NULL) return false;
    size_t escritos = fwrite(ctrl, sizeof(Controle), 1, f);
    fclose(f);
    return escritos == 1;
}

//Lê o controle e devolve false se o arquivo não existir
//(ou seja, a árvore ainda não foi criada)
bool carregarControle(Controle *ctrl) {
    FILE *f = fopen(ARQ_CONTROLE, "rb");
    if (f == NULL) return false;
    size_t lidos = fread(ctrl, sizeof(Controle), 1, f);
    fclose(f);
    return lidos == 1;
}

//SALVA E CARREGA NÓS
//Salva um nó no arquivo cujo nome esta em node->nomeArquivo
bool salvarNo(const BTreeNode *node, int T) {
    (void) T;
    FILE *f = fopen(node->nomeArquivo, "wb");
    if (f == NULL) return false;

    bool ok = true;
    ok = ok && fwrite(&node->n,    sizeof(int),  1, f) == 1;
    ok = ok && fwrite(&node->leaf, sizeof(bool), 1, f) == 1;

    ok = ok && fwrite(node->keys,  sizeof(int),      node->n, f) == (size_t) node->n;
    ok = ok && fwrite(node->dados, sizeof(Episodio), node->n, f) == (size_t) node->n;

    //filhos: se não for folha, tem n+1 filhos possiveis
    int qtdFilhos = node->leaf ? 0 : node->n + 1;
    for (int i = 0; i < qtdFilhos && ok; i++) {
        ok = ok && fwrite(node->children[i], sizeof(char), TAM_NOME, f) == TAM_NOME;
    }

    fclose(f);
    return ok;
}

//Lê um nó do arquivo, na mesma ordem em que foi gravado 
BTreeNode *carregarNo(const char *nomeArquivo, int T) {
    FILE *f = fopen(nomeArquivo, "rb");
    if (f == NULL) return NULL;

    BTreeNode *node = createNode(T, true);   //o leaf certo vem do arquivo, logo abaixo
    if (node == NULL) {
        fclose(f);
        return NULL;
    }
    snprintf(node->nomeArquivo, TAM_NOME, "%s", nomeArquivo);

    bool ok = true;
    ok = ok && fread(&node->n,    sizeof(int),  1, f) == 1;
    ok = ok && fread(&node->leaf, sizeof(bool), 1, f) == 1;

    ok = ok && fread(node->keys,  sizeof(int),      node->n, f) == (size_t) node->n;
    ok = ok && fread(node->dados, sizeof(Episodio), node->n, f) == (size_t) node->n;

    //filhos: se não for folha, tem n+1 filhos possiveis
    int qtdFilhos = node->leaf ? 0 : node->n + 1;
    for (int i = 0; i < qtdFilhos && ok; i++) {
        ok = ok && fread(node->children[i], sizeof(char), TAM_NOME, f) == TAM_NOME;
    }

    fclose(f);

    if (!ok) {
        freeNode(node, T);
        return NULL;
    }
    return node;
}

//Apaga do disco o arquivo do nó (usado quando dois nós viram um só)
void apagarNo(const BTreeNode *node) {
    remove(node->nomeArquivo);
}

//BUSCA

//Resultado da busca: achou ou não, e se achou, em qual nó e posição
typedef struct {
    bool achou;
    char arquivoNo[TAM_NOME];
    int  posicao;
} ResultadoBusca;

//Busca a chave na subárvore com raiz no nó cujo arquivo é nomeArquivoNo
ResultadoBusca buscar(const char *nomeArquivoNo, int chave, int T) {
    ResultadoBusca resultado;
    resultado.achou = false;
    resultado.posicao = -1;
    resultado.arquivoNo[0] = '\0';

    BTreeNode *node = carregarNo(nomeArquivoNo, T);
    if (node == NULL) {
        return resultado; //Não conseguiu ler o nó (erro ou árvore vazia)
    }

    //Procura a posição da chave (ou do filho que deve ser seguido)
    int i = 0;
    while (i < node->n && chave > node->keys[i]) {
        i++;
    }

    if (i < node->n && chave == node->keys[i]) {
        //Encontrado
        resultado.achou = true;
        snprintf(resultado.arquivoNo, TAM_NOME, "%s", nomeArquivoNo);
        resultado.posicao = i;
        freeNode(node, T);
        return resultado;
    }

    if (node->leaf) {
        //A chave não existe na árvore
        freeNode(node, T);
        return resultado;
    }

    //Desce pro filho i
    char proximoArquivo[TAM_NOME];
    snprintf(proximoArquivo, TAM_NOME, "%s", node->children[i]);
    freeNode(node, T); //Libera pois já não será mais usado

    return buscar(proximoArquivo, chave, T);
}

//IMPRESSÃO
#define LARGURA_TELA 118  //Se passar disso lista só os níveis, sem desenho
#define GAP_NOS        3  //Espaco entre dois nos vizinhos no desenho
#define PREFIXO       10  //Colunas reservadas para o rotulo "Nivel N:" a esquerda

//Nó da árvore de desenho (para imprimir por níveis)
typedef struct NoImp {
    int            n;            //N° de chaves
    char           texto[512];   //Texto do nó (chaves e barras)
    int            largura;      //Tamanho do texto
    int            nFilhos;
    struct NoImp **filhos;
    int            larguraSub;   //Largura que a subarvore inteira ocupa
    int            x;            //Coluna onde o texto do nó começa
} NoImp;

//Libera o NoImp e os filhos dele
void liberarNoImp(NoImp *p) {
    if (p == NULL) return;
    for (int i = 0; i < p->nFilhos; i++) {
        liberarNoImp(p->filhos[i]);
    }
    free(p->filhos);
    free(p);
}

//Lê a árvore do disco e monta a cópia para o desenho. Devolve NULL se der erro
NoImp *construirNoImp(const char *nomeArquivo, int T) {
    BTreeNode *node = carregarNo(nomeArquivo, T);
    if (node == NULL) return NULL;

    NoImp *p = (NoImp *) calloc(1, sizeof(NoImp));
    if (p == NULL) {
        freeNode(node, T);
        return NULL;
    }

    p->n = node->n;
    int pos = snprintf(p->texto, sizeof(p->texto), "|");
    for (int i = 0; i < node->n && pos < (int) sizeof(p->texto) - 16; i++) {
        pos += snprintf(p->texto + pos, sizeof(p->texto) - pos, " %d |", node->keys[i]);
    }
    p->largura = (int) strlen(p->texto);

    if (!node->leaf) {
        p->nFilhos = node->n + 1;
        p->filhos = (NoImp **) calloc(p->nFilhos, sizeof(NoImp *));
        if (p->filhos == NULL) {
            p->nFilhos = 0;
            liberarNoImp(p);
            freeNode(node, T);
            return NULL;
        }
        for (int i = 0; i < p->nFilhos; i++) {
            p->filhos[i] = construirNoImp(node->children[i], T);
            if (p->filhos[i] == NULL) {
                liberarNoImp(p);
                freeNode(node, T);
                return NULL;
            }
        }
    }

    freeNode(node, T);
    return p;
}

//Calcula, de baixo para cima, quanta largura cada subarvore precisa
void calcularLargura(NoImp *p) {
    int soma = 0;
    for (int i = 0; i < p->nFilhos; i++) {
        calcularLargura(p->filhos[i]);
        soma += p->filhos[i]->larguraSub;
        if (i > 0) soma += GAP_NOS;
    }
    p->larguraSub = (soma > p->largura) ? soma : p->largura;
}

//Decide, de cima para baixo, em que coluna cada no fica
void posicionar(NoImp *p, int inicio) {
    p->x = inicio + (p->larguraSub - p->largura) / 2;
    if (p->nFilhos == 0) return;

    int total = 0;
    for (int i = 0; i < p->nFilhos; i++) {
        total += p->filhos[i]->larguraSub;
        if (i > 0) total += GAP_NOS;
    }
    int cursor = inicio + (p->larguraSub - total) / 2;
    for (int i = 0; i < p->nFilhos; i++) {
        posicionar(p->filhos[i], cursor);
        cursor += p->filhos[i]->larguraSub + GAP_NOS;
    }
}

//Número de níveis (numa arvore B todas as folhas ficam na mesma altura).
int alturaImp(const NoImp *p) {
    return (p->nFilhos == 0) ? 1 : 1 + alturaImp(p->filhos[0]);
}

//Escreve um texto na "tela" (matriz de caracteres) a partir da linha e coluna
void escrever(char *tela, int larguraTela, int linha, int coluna, const char *texto) {
    for (int i = 0; texto[i] != '\0'; i++) {
        tela[linha * (larguraTela + 1) + coluna + i] = texto[i];
    }
}

//Desenha o nó, os filhos e as linhas que ligam pai e filhos
//Cada nível usa 2 linhas da tela: o texto do nó e a linha com as barras / | \ (como no exemplo do PDF)
void desenhar(const NoImp *p, char *tela, int larguraTela, int nivel) {
    int w = larguraTela + 1;     //Tamanho de uma linha da matriz (com o '\0')
    int linha = 2 * nivel;

    escrever(tela, larguraTela, linha, p->x, p->texto);
    if (p->nFilhos == 0) return;

    int centroPai = p->x + p->largura / 2;
    for (int i = 0; i < p->nFilhos; i++) {
        int centro = p->filhos[i]->x + p->filhos[i]->largura / 2;
        char barra = '|';
        if (centro < centroPai) barra = '/';
        if (centro > centroPai) barra = '\\';
        tela[(linha + 1) * w + (centroPai + centro) / 2] = barra;
        desenhar(p->filhos[i], tela, larguraTela, nivel + 1);
    }
}

//Escreve só os nós de um nível, da esquerda para a direita (usada quando a árvore não cabe no desenho)
void listarNivel(const NoImp *p, int atual, int alvo) {
    if (atual == alvo) {
        printf("%s  ", p->texto);
        return;
    }
    for (int i = 0; i < p->nFilhos; i++) {
        listarNivel(p->filhos[i], atual + 1, alvo);
    }
}

//Imprime a árvore por níveis, como no exemplo do PDF
//Se não couber na tela, imprime só os nós de cada nível, um nível por linha (o PDF avisa que só imprime bem com t pequeno)
void imprimirArvore(const char *nomeArquivoRaiz, int T) {
    NoImp *raiz = construirNoImp(nomeArquivoRaiz, T);
    if (raiz == NULL) {
        printf("Erro ao ler a arvore.\n");
        return;
    }

    //Árvore vazia
    if (raiz->n == 0 && raiz->nFilhos == 0) {
        printf("\n===== Arvore (nivel por nivel) =====\n(arvore vazia)\n");
        liberarNoImp(raiz);
        return;
    }

    //Calcula a largura e, se passar do limite, imprime só os nós de cada nível
    calcularLargura(raiz);
    int larguraTela = PREFIXO + raiz->larguraSub;
    int altura = alturaImp(raiz);

    if (larguraTela > LARGURA_TELA) {
        printf("\n(Arvore grande demais para desenhar na tela, mostrando so os niveis)\n");
        for (int nivel = 0; nivel < altura; nivel++) {
            printf("Nivel %d: ", nivel + 1);
            listarNivel(raiz, 0, nivel);
            printf("\n");
        }
        liberarNoImp(raiz);
        return;
    }

    int linhas = 2 * (altura - 1) + 1;
    int w = larguraTela + 1;

    //A tela é uma matriz de caracteres que começa toda de espaços
    char *tela = (char *) malloc((size_t) linhas * w);
    if (tela == NULL) {
        liberarNoImp(raiz);
        printf("Erro: memoria insuficiente.\n");
        return;
    }
    memset(tela, ' ', (size_t) linhas * w);
    for (int l = 0; l < linhas; l++) {
        tela[l * w + larguraTela] = '\0';
    }

    //Posiciona, desenha e escreve o "Nivel N:" de cada nível
    posicionar(raiz, PREFIXO);
    desenhar(raiz, tela, larguraTela, 0);
    for (int nivel = 0; nivel < altura; nivel++) {
        char rotulo[32];
        snprintf(rotulo, sizeof(rotulo), "Nivel %d:", nivel + 1);
        escrever(tela, larguraTela, 2 * nivel, 0, rotulo);
    }

    //Imprime linha por linha
    printf("\n===== Arvore (nivel por nivel) =====\n");
    for (int l = 0; l < linhas; l++) {
        char *linhaTela = tela + l * w;
        int fim = (int) strlen(linhaTela);
        while (fim > 0 && linhaTela[fim - 1] == ' ') fim--;   //Tira os espaços do final
        linhaTela[fim] = '\0';
        printf("%s\n", linhaTela);
    }

    free(tela);
    liberarNoImp(raiz);
}

//INSERÇÃO (CLRS)

//Divide o filho cheio y (filho de número i do nó x)
//x não pode estar cheio e y tem 2T-1 chaves
//y fica com as T-1 primeiras, o nó novo z fica com as T-1 últimas e a chave do meio sobe para x
//Salva x, y e z no disco (retorna true se deu certo)
bool dividirFilho(Controle *ctrl, BTreeNode *x, int i, BTreeNode *y) {
    int T = ctrl->T;

    BTreeNode *z = createNode(T, y->leaf);
    if (z == NULL) return false;
    gerarNomeArquivo(ctrl, z->nomeArquivo);

    //z fica com as últimas T-1 chaves (e episódios) de y
    z->n = T - 1;
    for (int j = 0; j < T - 1; j++) {
        z->keys[j]  = y->keys[j + T];
        z->dados[j] = y->dados[j + T];
    }

    //Se y não é folha, z também leva os últimos T filhos
    if (!y->leaf) {
        for (int j = 0; j < T; j++) {
            snprintf(z->children[j], TAM_NOME, "%s", y->children[j + T]);
        }
    }

    y->n = T - 1;  //y fica só com a primeira metade

    //Abre espaço em x para o filho novo z (posição i+1)
    for (int j = x->n; j >= i + 1; j--) {
        snprintf(x->children[j + 1], TAM_NOME, "%s", x->children[j]);
    }
    snprintf(x->children[i + 1], TAM_NOME, "%s", z->nomeArquivo);

    //Abre espaço em x para a chave do meio de y (posição i), que sobe
    for (int j = x->n - 1; j >= i; j--) {
        x->keys[j + 1]  = x->keys[j];
        x->dados[j + 1] = x->dados[j];
    }
    x->keys[i]  = y->keys[T - 1];
    x->dados[i] = y->dados[T - 1];
    x->n++;

    bool ok = salvarNo(y, T) && salvarNo(z, T) && salvarNo(x, T);
    freeNode(z, T);
    return ok;
}

//Insere k (com o episódio ep) na subárvore de x, que não pode estar cheio
//Vai descendo e divide os filhos cheios antes de entrar neles, por isso nunca precisa voltar para cima
bool inserirNaoCheio(Controle *ctrl, BTreeNode *x, int k, const Episodio *ep) {
    int T = ctrl->T;
    int i = x->n - 1;

    if (x->leaf) {
        //Folha: empurra as chaves maiores para a direita e coloca o k
        while (i >= 0 && k < x->keys[i]) {
            x->keys[i + 1]  = x->keys[i];
            x->dados[i + 1] = x->dados[i];
            i--;
        }
        x->keys[i + 1]  = k;
        x->dados[i + 1] = *ep;
        x->n++;
        return salvarNo(x, T);
    }

    //Nó interno: descobre por qual filho o k tem que descer
    while (i >= 0 && k < x->keys[i]) {
        i--;
    }
    i++;

    BTreeNode *c = carregarNo(x->children[i], T);
    if (c == NULL) return false;

    if (c->n == 2 * T - 1) {
        //Filho cheio: divide antes de descer
        if (!dividirFilho(ctrl, x, i, c)) {
            freeNode(c, T);
            return false;
        }
        //A chave do meio subiu para x->keys[i], então vê se o k vai para o c (esquerda) ou para o z novo (direita)
        if (k > x->keys[i]) {
            freeNode(c, T);
            c = carregarNo(x->children[i + 1], T);
            if (c == NULL) return false;
        }
    }

    bool ok = inserirNaoCheio(ctrl, c, k, ep);
    freeNode(c, T);
    return ok;
}

//Insere um episódio na árvore
//Retorna 1 se inseriu, 0 se a chave já existia e -1 se deu erro
int inserir(Controle *ctrl, int chave, const Episodio *ep) {
    int T = ctrl->T;

    if (buscar(ctrl->raiz, chave, T).achou) {
        return 0;  //Árvore B não aceita chave repetida
    }

    BTreeNode *r = carregarNo(ctrl->raiz, T);
    if (r == NULL) return -1;

    bool ok;
    if (r->n == 2 * T - 1) {
        //Raiz cheia: a árvore cresce um nível
        //Cria uma raiz nova (s) que tem a raiz antiga como filho 0 e divide esse filho
        BTreeNode *s = createNode(T, false);
        if (s == NULL) {
            freeNode(r, T);
            return -1;
        }
        gerarNomeArquivo(ctrl, s->nomeArquivo);
        s->n = 0;
        snprintf(s->children[0], TAM_NOME, "%s", r->nomeArquivo);

        ok = dividirFilho(ctrl, s, 0, r);
        if (ok) {
            snprintf(ctrl->raiz, TAM_NOME, "%s", s->nomeArquivo);  //s é a raiz agora
            ok = inserirNaoCheio(ctrl, s, chave, ep);
        }
        freeNode(s, T);
    } else {
        ok = inserirNaoCheio(ctrl, r, chave, ep);
    }
    freeNode(r, T);

    //O contador e a raiz podem ter mudado, então salva o controle
    if (!ok || !salvarControle(ctrl)) return -1;
    return 1;
}

//Lê um .txt com linhas "chave;titulo;duracao;nota" e insere cada episódio
//No final mostra um resumo
void carregarDoTxt(Controle *ctrl, const char *nomeArquivo) {
    FILE *f = fopen(nomeArquivo, "r");
    if (f == NULL) {
        printf("Nao consegui abrir '%s'. Ele esta na mesma pasta do programa?\n",
               nomeArquivo);
        return;
    }

    char linha[256];
    int inseridos = 0, repetidos = 0, erros = 0;

    while (fgets(linha, sizeof(linha), f) != NULL) {
        int chave, duracao;
        float nota;
        Episodio ep;
        memset(&ep, 0, sizeof(ep));

        //O %63[^;] lê o título até o próximo ;
        if (sscanf(linha, "%d;%63[^;];%d;%f", &chave, ep.titulo, &duracao, &nota) != 4) {
            continue;  //Linha em branco ou fora do formato, ignora
        }
        ep.duracao = duracao;
        ep.nota = nota;

        int r = inserir(ctrl, chave, &ep);
        if (r == 1)      inseridos++;
        else if (r == 0) repetidos++;
        else             erros++;
    }
    fclose(f);

    printf("Carregamento terminado: %d inseridos, %d repetidos, %d erros.\n",
           inseridos, repetidos, erros);
}

//REMOÇÃO (CLRS)

//Acha a maior chave de uma subárvore: é a última chave da folha mais à direita
//Copia a chave e o episódio para chave e ep (retorna false se der erro)
bool chaveMaxima(const char *nomeArquivo, int T, int *chave, Episodio *ep) {
    char atual[TAM_NOME];
    snprintf(atual, TAM_NOME, "%s", nomeArquivo);
    while (true) {
        BTreeNode *node = carregarNo(atual, T);
        if (node == NULL) return false;
        if (node->leaf) {
            *chave = node->keys[node->n - 1];
            *ep    = node->dados[node->n - 1];
            freeNode(node, T);
            return true;
        }
        snprintf(atual, TAM_NOME, "%s", node->children[node->n]);  //Desce pelo último filho
        freeNode(node, T);
    }
}

//O contrário: a menor chave é a primeira da folha mais à esquerda
bool chaveMinima(const char *nomeArquivo, int T, int *chave, Episodio *ep) {
    char atual[TAM_NOME];
    snprintf(atual, TAM_NOME, "%s", nomeArquivo);
    while (true) {
        BTreeNode *node = carregarNo(atual, T);
        if (node == NULL) return false;
        if (node->leaf) {
            *chave = node->keys[0];
            *ep    = node->dados[0];
            freeNode(node, T);
            return true;
        }
        snprintf(atual, TAM_NOME, "%s", node->children[0]);  //Desce pelo primeiro filho
        freeNode(node, T);
    }
}

//Junta y (filho i), a chave x->keys[i] e z (filho i+1) em um nó só, o y
//Os dois têm T-1 chaves, então y fica com 2T-1
//O arquivo do z é apagado e o x perde a chave i e o filho i+1
bool fundirFilhos(Controle *ctrl, BTreeNode *x, int i, BTreeNode *y, BTreeNode *z) {
    int T = ctrl->T;

    //A chave do pai desce para o meio do y
    y->keys[T - 1]  = x->keys[i];
    y->dados[T - 1] = x->dados[i];

    //As chaves do z vão para o fim do y
    for (int j = 0; j < T - 1; j++) {
        y->keys[T + j]  = z->keys[j];
        y->dados[T + j] = z->dados[j];
    }
    //Se não forem folhas, os filhos do z também vão para o y
    if (!y->leaf) {
        for (int j = 0; j < T; j++) {
            snprintf(y->children[T + j], TAM_NOME, "%s", z->children[j]);
        }
    }
    y->n = 2 * T - 1;

    //O x perde a chave i (as outras andam uma casa para a esquerda)
    for (int j = i; j < x->n - 1; j++) {
        x->keys[j]  = x->keys[j + 1];
        x->dados[j] = x->dados[j + 1];
    }
    //E perde o filho i+1 (o z, que não existe mais)
    for (int j = i + 1; j < x->n; j++) {
        snprintf(x->children[j], TAM_NOME, "%s", x->children[j + 1]);
    }
    x->n--;

    bool ok = salvarNo(y, T) && salvarNo(x, T);
    apagarNo(z);   //O arquivo do z não serve mais
    return ok;
}

//O c (filho i do x) tem T-1 chaves e o irmão da esquerda tem T ou mais
//A chave do pai desce para o c e a última chave do irmão sobe para o pai
bool emprestarDoEsquerdo(Controle *ctrl, BTreeNode *x, int i,
                         BTreeNode *c, BTreeNode *esq) {
    int T = ctrl->T;

    //Abre espaço no começo do c
    for (int j = c->n - 1; j >= 0; j--) {
        c->keys[j + 1]  = c->keys[j];
        c->dados[j + 1] = c->dados[j];
    }
    if (!c->leaf) {
        for (int j = c->n; j >= 0; j--) {
            snprintf(c->children[j + 1], TAM_NOME, "%s", c->children[j]);
        }
    }

    c->keys[0]  = x->keys[i - 1];            //A chave do pai desce
    c->dados[0] = x->dados[i - 1];

    x->keys[i - 1]  = esq->keys[esq->n - 1]; //A última do irmão sobe
    x->dados[i - 1] = esq->dados[esq->n - 1];

    if (!c->leaf) {  //O último filho do irmão vira o primeiro do c
        snprintf(c->children[0], TAM_NOME, "%s", esq->children[esq->n]);
    }

    esq->n--;
    c->n++;
    return salvarNo(esq, T) && salvarNo(c, T) && salvarNo(x, T);
}

//O contrário: a chave do pai desce para o fim do c
//e a primeira chave do irmão da direita sobe para o pai
bool emprestarDoDireito(Controle *ctrl, BTreeNode *x, int i,
                        BTreeNode *c, BTreeNode *dir) {
    int T = ctrl->T;

    c->keys[c->n]  = x->keys[i];             //A chave do pai desce
    c->dados[c->n] = x->dados[i];

    x->keys[i]  = dir->keys[0];              //A primeira do irmão sobe
    x->dados[i] = dir->dados[0];

    if (!c->leaf) {  //O primeiro filho do irmão vira o último do c
        snprintf(c->children[c->n + 1], TAM_NOME, "%s", dir->children[0]);
    }

    //O irmão fecha o buraco que ficou no começo
    for (int j = 0; j < dir->n - 1; j++) {
        dir->keys[j]  = dir->keys[j + 1];
        dir->dados[j] = dir->dados[j + 1];
    }
    if (!dir->leaf) {
        for (int j = 0; j < dir->n; j++) {
            snprintf(dir->children[j], TAM_NOME, "%s", dir->children[j + 1]);
        }
    }

    dir->n--;
    c->n++;
    return salvarNo(dir, T) && salvarNo(c, T) && salvarNo(x, T);
}

//Remove a chave k da subárvore de x
//Regra do CLRS: antes de descer para um nó, garante que ele tem pelo menos T chaves
//(só a raiz da árvore inteira pode ter menos)
bool removerDoNo(Controle *ctrl, BTreeNode *x, int k) {
    int T = ctrl->T;

    int i = 0;
    while (i < x->n && k > x->keys[i]) {
        i++;
    }

    if (i < x->n && k == x->keys[i]) {
        //A chave está neste nó

        if (x->leaf) {
            //Caso 1: está numa folha, só apaga e fecha o buraco
            for (int j = i; j < x->n - 1; j++) {
                x->keys[j]  = x->keys[j + 1];
                x->dados[j] = x->dados[j + 1];
            }
            x->n--;
            return salvarNo(x, T);
        }

        //Caso 2: está num nó interno, olha os dois filhos que ficam ao lado da chave
        BTreeNode *y = carregarNo(x->children[i], T);      //Filho da esquerda
        BTreeNode *z = carregarNo(x->children[i + 1], T);  //Filho da direita
        bool ok = (y != NULL && z != NULL);

        if (ok && y->n >= T) {
            //Caso 2a: a esquerda tem chave de sobra
            //Troca o k pelo predecessor (a maior chave da esquerda) e apaga o predecessor lá embaixo
            int chavePred; Episodio epPred;
            ok = chaveMaxima(y->nomeArquivo, T, &chavePred, &epPred);
            if (ok) {
                x->keys[i]  = chavePred;
                x->dados[i] = epPred;
                ok = salvarNo(x, T) && removerDoNo(ctrl, y, chavePred);
            }
        } else if (ok && z->n >= T) {
            //Caso 2b: a direita tem chave de sobra
            //Troca o k pelo sucessor (a menor chave da direita) e apaga o sucessor lá embaixo
            int chaveSuc; Episodio epSuc;
            ok = chaveMinima(z->nomeArquivo, T, &chaveSuc, &epSuc);
            if (ok) {
                x->keys[i]  = chaveSuc;
                x->dados[i] = epSuc;
                ok = salvarNo(x, T) && removerDoNo(ctrl, z, chaveSuc);
            }
        } else if (ok) {
            //Caso 2c: os dois filhos têm só T-1 chaves
            //Junta y + k + z em um nó só e apaga o k lá dentro
            ok = fundirFilhos(ctrl, x, i, y, z) && removerDoNo(ctrl, y, k);
        }

        freeNode(y, T);
        freeNode(z, T);
        return ok;
    }

    //A chave não está neste nó

    if (x->leaf) {
        return false;  //Chegou numa folha sem achar, a chave não existe
    }

    //Caso 3: se a chave existe, está na subárvore do filho i
    //Antes de descer, garante que esse filho tem pelo menos T chaves
    BTreeNode *c   = carregarNo(x->children[i], T);
    BTreeNode *esq = NULL;   //Irmão da esquerda
    BTreeNode *dir = NULL;   //Irmão da direita
    BTreeNode *alvo = c;     //Nó por onde vai continuar descendo
    bool ok = (c != NULL);

    if (ok && c->n == T - 1) {
        if (i > 0)    esq = carregarNo(x->children[i - 1], T);
        if (i < x->n) dir = carregarNo(x->children[i + 1], T);
        ok = !((i > 0 && esq == NULL) || (i < x->n && dir == NULL));

        if (ok && esq != NULL && esq->n >= T) {
            //Caso 3a: o irmão da esquerda tem de sobra, pede emprestado
            ok = emprestarDoEsquerdo(ctrl, x, i, c, esq);
        } else if (ok && dir != NULL && dir->n >= T) {
            //Caso 3a: o irmão da direita tem de sobra, pede emprestado
            ok = emprestarDoDireito(ctrl, x, i, c, dir);
        } else if (ok && dir != NULL) {
            //Caso 3b: nenhum irmão tem de sobra, funde o c com o da direita
            ok = fundirFilhos(ctrl, x, i, c, dir);
        } else if (ok) {
            //Caso 3b: o c é o último filho, funde com o da esquerda
            ok = fundirFilhos(ctrl, x, i - 1, esq, c);
            alvo = esq;  //Depois da fusão o nó que sobrou é o esq
        }
    }

    if (ok) ok = removerDoNo(ctrl, alvo, k);

    freeNode(c, T);
    freeNode(esq, T);
    freeNode(dir, T);
    return ok;
}

//Remove um episódio da árvore
//Retorna 1 se removeu, 0 se a chave não existe e -1 se deu erro
int remover(Controle *ctrl, int chave) {
    int T = ctrl->T;

    if (!buscar(ctrl->raiz, chave, T).achou) {
        return 0;
    }

    BTreeNode *r = carregarNo(ctrl->raiz, T);
    if (r == NULL) return -1;
    bool ok = removerDoNo(ctrl, r, chave);
    freeNode(r, T);
    if (!ok) return -1;

    //Se a raiz ficou sem chaves mas ainda tem filho, a árvore perde um nível
    //O filho que sobrou vira a raiz
    BTreeNode *raizAtual = carregarNo(ctrl->raiz, T);
    if (raizAtual == NULL) return -1;
    if (raizAtual->n == 0 && !raizAtual->leaf) {
        char filho[TAM_NOME];
        snprintf(filho, TAM_NOME, "%s", raizAtual->children[0]);
        apagarNo(raizAtual);                       //Apaga o arquivo da raiz antiga
        snprintf(ctrl->raiz, TAM_NOME, "%s", filho);
    }
    freeNode(raizAtual, T);

    return salvarControle(ctrl) ? 1 : -1;
}

//PROGRAMA PRINCIPAL
int main(void) {
    Controle ctrl;
    memset(&ctrl, 0, sizeof(ctrl));  //Zera tudo para não gravar lixo de memória no controle.bin
    bool arvoreJaExistia = carregarControle(&ctrl);

    //Se já tem controle.bin reaproveita a árvore, senão cria uma nova
    if (arvoreJaExistia) {
        printf("Arvore existente encontrada (controle.bin).\n");
        printf("Grau minimo t = %d\n", ctrl.T);
    } else {
        printf("Nenhuma arvore encontrada. Vamos criar uma nova.\n");
        printf("Digite o grau minimo t da arvore B (t >= 2): ");
        if (scanf("%d", &ctrl.T) != 1 || ctrl.T < 2) {
            printf("Valor invalido. O grau minimo precisa ser um inteiro >= 2.\n");
            return 1;
        }

        ctrl.proximoNumero = 1;

        //A raiz nova começa vazia e é folha
        BTreeNode *raiz = createNode(ctrl.T, true);
        if (raiz == NULL) {
            printf("Erro: memoria insuficiente.\n");
            return 1;
        }

        gerarNomeArquivo(&ctrl, raiz->nomeArquivo);
        snprintf(ctrl.raiz, TAM_NOME, "%s", raiz->nomeArquivo);

        if (!salvarNo(raiz, ctrl.T) || !salvarControle(&ctrl)) {
            printf("Erro ao salvar a arvore nova.\n");
            freeNode(raiz, ctrl.T);
            return 1;
        }

        printf("Arvore criada. Raiz salva em '%s'.\n", raiz->nomeArquivo);
        freeNode(raiz, ctrl.T);
    }

    int opcao;
    do {
        printf("\n===== Guia do Arco do Red John - Arvore B =====\n");
        printf("1. Buscar episodio\n");
        printf("2. Inserir episodio\n");
        printf("3. Remover episodio\n");
        printf("4. Imprimir arvore (por niveis)\n");
        printf("5. Carregar episodios do arquivo episodios_red_john.txt\n");
        printf("0. Sair\n");
        printf("Escolha: ");

        if (scanf("%d", &opcao) != 1) {
            printf("Entrada invalida.\n");
            break;
        }

        switch (opcao) {
            case 1: {
                int chaveBuscada;
                printf("Digite a chave do episodio (ex: 208 para T2E8): ");
                scanf("%d", &chaveBuscada);

                ResultadoBusca r = buscar(ctrl.raiz, chaveBuscada, ctrl.T);
                if (r.achou) {
                    BTreeNode *noAchado = carregarNo(r.arquivoNo, ctrl.T);
                    Episodio *ep = &noAchado->dados[r.posicao];
                    printf("Encontrado em '%s': %s (%d min, nota %.1f)\n",
                           r.arquivoNo, ep->titulo, ep->duracao, ep->nota);
                    freeNode(noAchado, ctrl.T);
                } else {
                    printf("Episodio com chave %d nao encontrado.\n", chaveBuscada);
                }
                break;
            }
            case 2: {
                int chave;
                Episodio ep;
                memset(&ep, 0, sizeof(ep));

                printf("Chave (temporada*100 + episodio, ex: 305 para T3E5): ");
                scanf("%d", &chave);
                printf("Titulo: ");
                scanf(" %63[^\n]", ep.titulo);  //O espaço antes do % faz o scanf pular o Enter que sobrou
                printf("Duracao em minutos: ");
                scanf("%d", &ep.duracao);
                printf("Nota (ex: 8.5): ");
                scanf("%f", &ep.nota);

                int r = inserir(&ctrl, chave, &ep);
                if (r == 1)      printf("Episodio %d inserido.\n", chave);
                else if (r == 0) printf("Ja existe um episodio com a chave %d.\n", chave);
                else             printf("Erro ao inserir.\n");
                break;
            }
            case 3: {
                int chave;
                printf("Digite a chave do episodio a remover (ex: 208 para T2E8): ");
                scanf("%d", &chave);

                int r = remover(&ctrl, chave);
                if (r == 1)      printf("Episodio %d removido.\n", chave);
                else if (r == 0) printf("Episodio com chave %d nao encontrado.\n", chave);
                else             printf("Erro ao remover.\n");
                break;
            }
            case 4:
                imprimirArvore(ctrl.raiz, ctrl.T);
                break;
            case 5:
                carregarDoTxt(&ctrl, "episodios_red_john.txt");
                break;
            case 0:
                printf("Ate mais!\n");
                break;
            default:
                printf("Opcao invalida.\n");
        }
    } while (opcao != 0);

    return 0;
}