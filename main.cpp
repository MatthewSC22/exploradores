#include <iostream>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <stack>
#include <queue>
#include <climits>
#include <chrono>
#include <iomanip>
#include <random>
#include <windows.h> 

using namespace std;
using namespace std::chrono;

class Card {
public:
    int value;
    int color;

    Card(int v, int c) : value(v), color(c) {}
};

class Player {
public:
    vector<Card> hand;
    vector<stack<Card>> expeditions;

    Player() {
        expeditions.resize(5);
    }

    void drawCard(Card card) {
        hand.push_back(card);
        sortHand();
    }

    void sortHand() {
        sort(hand.begin(), hand.end(), [](const Card& a, const Card& b) {
            return a.value < b.value;
        });
    }

    bool canPlayCard(int cardIndex, int expeditionIndex) {
        const Card& card = hand[cardIndex];
        stack<Card>& expedition = expeditions[expeditionIndex];

        if (expedition.empty()) {
            return card.value == -1 || card.value >= 2;
        } else {
            return (card.value == -1 && expedition.top().value == -1) || (expedition.top().value < card.value);
        }
    }

    void playCard(int cardIndex, int expeditionIndex) {
        if (canPlayCard(cardIndex, expeditionIndex)) {
            expeditions[expeditionIndex].push(hand[cardIndex]);
            hand.erase(hand.begin() + cardIndex);
        } else {
            //cout << "Movimento invalido! Carta nao pode ser jogada." << endl;
            playCard(cardIndex, expeditionIndex);
        }
    }

    void discardCard(int cardIndex, vector<queue<Card>>& discardPiles) {
        discardPiles[hand[cardIndex].color].push(hand[cardIndex]);
        hand.erase(hand.begin() + cardIndex);
    }

    void drawFromDiscard(int color, vector<queue<Card>>& discardPiles) {
        if (!discardPiles[color].empty()) {
            hand.push_back(discardPiles[color].front());
            discardPiles[color].pop();
            sortHand();
        }
    }

    int chooseCardToPlayForPlayer1() {
        int minCardValue = INT_MAX;
        int cardIndex = -1;

        for (int i = 0; i < hand.size(); ++i) {
            if (hand[i].value < minCardValue) {
                minCardValue = hand[i].value;
                cardIndex = i;
            }
        }

        return cardIndex;
    }

    int chooseCardToPlayForPlayer2() {
        int maxCardValue = INT_MIN;
        int cardIndex = -1;

        for (int i = 0; i < hand.size(); ++i) {
            if (hand[i].value > maxCardValue) {
                maxCardValue = hand[i].value;
                cardIndex = i;
            }
        }

        return cardIndex;
    }

    int calculatePotentialPoints(int cardIndex, int expeditionIndex) {
        const Card &card = hand[cardIndex];
        const std::stack<Card> &expedition = expeditions[expeditionIndex];
        
        int currentPoints = 0;
        int multiplier = 1;  // Começa com multiplicador 1 (nenhuma carta especial)
        int numberOfCards = 0;

        // Calcula a pontuação atual da expedição
        std::stack<Card> temp = expedition;
        while (!temp.empty()) {
            const Card &topCard = temp.top();
            if (topCard.value == 0) {  // Carta especial (multiplicador)
                multiplier++;
            } else {
                currentPoints += topCard.value;
            }
            temp.pop();
            numberOfCards++;
        }

        // Adiciona os pontos da nova carta
        if (card.value == 0) {  // Se for uma carta especial (multiplicador)
            multiplier++;
        } else {
            currentPoints += card.value;
        }
        numberOfCards++;

        // Calcula a pontuação potencial
        int potentialPoints = (currentPoints - 20) * multiplier;

        // Aplica bônus por ter pelo menos 8 cartas na expedição
        if (numberOfCards >= 8) {
            potentialPoints += 20;
        }

        return potentialPoints;
    }
};

class Game {
public:
    vector<Card> deck;
    vector<queue<Card>> discardPiles;
    Player players[2];
    int currentPlayer;

    Game() {
        discardPiles.resize(5);
        currentPlayer = 0;
        initializeDeck();
        shuffleDeck();
        dealInitialCards();
    }

    void initializeDeck() {
        for (int color = 0; color < 5; ++color) {
            for (int value = 2; value <= 10; ++value) {
                deck.push_back(Card(value, color));
            }
            for (int i = 0; i < 3; ++i) {
                deck.push_back(Card(-1, color));
            }
        }
    }

    void shuffleDeck() {
        srand(time(0));
        random_shuffle(deck.begin(), deck.end());
        // Inicializa o gerador de números aleatórios com a semente atual do tempo
        //unsigned seed = static_cast<unsigned>(time(0));
        //default_random_engine rng(seed);
        // Embaralha o baralho usando std::shuffle
        //shuffle(deck.begin(), deck.end(), rng);
    }

    void dealInitialCards() {
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 8; ++j) {
                players[i].drawCard(deck.back());
                deck.pop_back();
            }
        }
    }

    void drawCardFromDeck() {
        if (!deck.empty()) {
            players[currentPlayer].drawCard(deck.back());
            deck.pop_back();
        }
    }

    //com 10 cartas ou menos priorizar fazer maior numero de pontos
    void takeTurn() {
        //cout << "Jogador " << currentPlayer + 1 << " turno:" << endl;

        // Mostrar a mao
        /*cout << "Mao: ";
        for (size_t i = 0; i < players[currentPlayer].hand.size(); ++i) {
            cout << players[currentPlayer].hand[i].value << "-" << players[currentPlayer].hand[i].color << " ";
        }
        cout << endl;*/

        int cardIndex;
        if (currentPlayer == 0) {
            cardIndex = players[currentPlayer].chooseCardToPlayForPlayer1();
        } else {
            cardIndex = players[currentPlayer].chooseCardToPlayForPlayer2();
        }

        int expeditionIndex = -1;

        if (deck.size() > 10) {
            // Quando o deck tem mais de 10 cartas, a IA escolhe a primeira expedição válida.
            for (int i = 0; i < 5; ++i) {
                if (players[currentPlayer].canPlayCard(cardIndex, i)) {
                    expeditionIndex = i;
                    break;
                }
            }
        } else {
            // Quando o deck tem 10 ou menos cartas, a IA prioriza maximizar pontos.
            int maxPoints = -1000;  // Valor inicial baixo para encontrar a melhor opção.
            for (int i = 0; i < 5; ++i) {
                if (players[currentPlayer].canPlayCard(cardIndex, i)) {
                    int potentialPoints = players[currentPlayer].calculatePotentialPoints(cardIndex, i);
                    if (potentialPoints > maxPoints) {
                        maxPoints = potentialPoints;
                        expeditionIndex = i;
                    }
                }
            }
        }

        // Jogar ou descartar a carta com base na expedição escolhida.
        if (expeditionIndex >= 0 && expeditionIndex < 5) {
            players[currentPlayer].playCard(cardIndex, expeditionIndex);
            //cout << "Jogador " << currentPlayer + 1 << " jogou carta " << players[currentPlayer].expeditions[expeditionIndex].top().value << "-" << players[currentPlayer].expeditions[expeditionIndex].top().color << " na expedicao " << expeditionIndex << "." << endl;
        } else {
            players[currentPlayer].discardCard(cardIndex, discardPiles);
            //cout << "Jogador " << currentPlayer + 1 << " descartou carta " << players[currentPlayer].hand[cardIndex].value << "-" << players[currentPlayer].hand[cardIndex].color << "." << endl;
        }

        // Mostrar as cartas no topo das pilhas de descarte.
        /*cout << "Cartas do topo da pilha de descarte: ";
        for (int i = 0; i < 5; ++i) {
            if (!discardPiles[i].empty()) {
                cout << discardPiles[i].front().value << "-" << discardPiles[i].front().color << " ";
            } else {
                cout << "Vazio ";
            }
        }
        cout << endl;*/

        // Decisão da IA para puxar do deck ou da pilha de descarte com a nova condição.
        if (deck.size() > 10) {
            int discardChoice = -1;
            for (int i = 0; i < 5; ++i) {
                if (!discardPiles[i].empty() && (currentPlayer == 0 ? discardPiles[i].front().value < 5 : discardPiles[i].front().value > 5)) {
                    discardChoice = i;
                    break;
                }
            }

            if (discardChoice >= 0) {
                players[currentPlayer].drawFromDiscard(discardChoice, discardPiles);
                //cout << "Jogador " << currentPlayer + 1 << " puxou da pilha de descarte. " << discardChoice << "." << endl;
            } else {
                drawCardFromDeck();
                //cout << "Jogador " << currentPlayer + 1 << " puxou do deck." << endl;
            }
        } else {
            // Puxar apenas do deck se restarem 10 ou menos cartas.
            drawCardFromDeck();
            //cout << "Jogador " << currentPlayer + 1 << " puxou do deck." << endl;
        }

        // Exibir o número de cartas restantes no deck.
        //cout << "Cartas sobrando no deck: " << deck.size() << endl;

        currentPlayer = 1 - currentPlayer;
    }



    // buscar priorizar a primeira expedição valida
    /*
    void takeTurn() {
        cout << "Jogador " << currentPlayer + 1 << " turno:" << endl;

        // Mostrar a mao
        cout << "Mao: ";
        for (size_t i = 0; i < players[currentPlayer].hand.size(); ++i) {
            cout << players[currentPlayer].hand[i].value << "-" << players[currentPlayer].hand[i].color << " ";
        }
        cout << endl;

        int cardIndex;
        if (currentPlayer == 0) {
            cardIndex = players[currentPlayer].chooseCardToPlayForPlayer1();
        } else {
            cardIndex = players[currentPlayer].chooseCardToPlayForPlayer2();
        }

        // Escolhe automaticamente a melhor expedicao para jogar a carta
        int expeditionIndex = -1;
        for (int i = 0; i < 5; ++i) {
            if (players[currentPlayer].canPlayCard(cardIndex, i)) {
                expeditionIndex = i;
                break;  // Escolhe a primeira expedicao valida
            }
        }

        if (expeditionIndex >= 0 && expeditionIndex < 5) {
            players[currentPlayer].playCard(cardIndex, expeditionIndex);
            cout << "Jogador " << currentPlayer + 1 << " jogou a carta " << players[currentPlayer].expeditions[expeditionIndex].top().value << "-" << players[currentPlayer].expeditions[expeditionIndex].top().color << " on expedition " << expeditionIndex << "." << endl;
        } else {
            players[currentPlayer].discardCard(cardIndex, discardPiles);
            cout << "Jogador " << currentPlayer + 1 << " descartou a carta " << players[currentPlayer].hand[cardIndex].value << "-" << players[currentPlayer].hand[cardIndex].color << "." << endl;
        }

        cout << "Cartas do topo da pilha de descarte: ";
        for (int i = 0; i < 5; ++i) {
            if (!discardPiles[i].empty()) {
                cout << discardPiles[i].front().value << "-" << discardPiles[i].front().color << " ";
            } else {
                cout << "Vazio ";
            }
        }
        cout << endl;

        // IA decide puxar do deck ou da pilha de descarte com a condicao
        if (deck.size() > 10) {
            int discardChoice = -1;
            for (int i = 0; i < 5; ++i) {
                if (!discardPiles[i].empty() && (currentPlayer == 0 ? discardPiles[i].front().value < 5 : discardPiles[i].front().value > 5)) {
                    discardChoice = i;
                    break;
                }
            }

            if (discardChoice >= 0) {
                players[currentPlayer].drawFromDiscard(discardChoice, discardPiles);
                cout << "Jogador " << currentPlayer + 1 << " puxou da pilha de descarte. " << discardChoice << "." << endl;
            } else {
                drawCardFromDeck();
                cout << "Jogador " << currentPlayer + 1 << " puxou do deck." << endl;
            }
        } else {
            // Puxa apenas do deck se tiver 10 cartas ou menos sobrando
            drawCardFromDeck();
            cout << "Jogador " << currentPlayer + 1 << " puxou do deck." << endl;
        }

        cout << "Cartas sobrando no deck: " << deck.size() << endl;

        currentPlayer = 1 - currentPlayer;
    }
    */

    
    bool isGameOver() {
        return deck.empty();
    }

    void playGame() {
        while (!isGameOver()) {
            takeTurn();
        }

        calculateScores();
    }

    void calculateScores() {
        for (int i = 0; i < 2; ++i) {
            int score = 0;
            for (const auto& expedition : players[i].expeditions) {
                int expeditionScore = 0;
                int investmentCards = 0;
                stack<Card> tempExpedition = expedition;
                while (!tempExpedition.empty()) {
                    if (tempExpedition.top().value == -1) {
                        investmentCards++;
                    } else {
                        expeditionScore += tempExpedition.top().value;
                    }
                    tempExpedition.pop();
                }
                if (!expedition.empty()) {
                    expeditionScore = (expeditionScore - 20) * (1 + investmentCards);
                }
                score += expeditionScore;
            }
            //cout << "Jogador " << i + 1 << " pontuacao: " << score << endl;
        }
    }
};

int main() {

    int numRodadas = 5;
    //cout << "Digite o numero de rodadas: ";
    //cin >> numRodadas;

    vector<double> temposDeExecucao(numRodadas);
    vector<pair<int, int>> resultados(numRodadas);  // Armazena pontuação dos jogadores

    for (int i = 0; i < numRodadas; ++i) {

        auto start = high_resolution_clock::now();
        pair<int, int> score = {INT_MIN, INT_MIN};  // Inicializa pontuações como nulas

        //Sleep(2000); // Espera dois segundos

        while (true) {
            auto now = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(now - start).count();

            if (duration > 15000) {  // 15000 ms = 15 segundos
                cout << "Rodada " << i + 1 << " excedeu o limite de 15 segundos. Jogo encerrado." << endl;
                temposDeExecucao[i] = 15.000;
                break;
            }

            // Criar um novo jogo para cada rodada
            Game game;
            game.playGame();

            // Calcular pontuações dos jogadores
            score = {0, 0};
            for (int j = 0; j < 2; ++j) {
                int playerScore = 0;
                for (const auto& expedition : game.players[j].expeditions) {
                    int expeditionScore = 0;
                    int investmentCards = 0;
                    stack<Card> tempExpedition = expedition;
                    while (!tempExpedition.empty()) {
                        if (tempExpedition.top().value == -1) {
                            investmentCards++;
                        } else {
                            expeditionScore += tempExpedition.top().value;
                        }
                        tempExpedition.pop();
                    }
                    if (!expedition.empty()) {
                        expeditionScore = (expeditionScore - 20) * (1 + investmentCards);
                    }
                    playerScore += expeditionScore;
                }
                if (j == 0) {
                    score.first = playerScore;
                } else {
                    score.second = playerScore;
                }
            }

            temposDeExecucao[i] = duration / 1000.0;  // Converter para segundos com precisão de milissegundos
            resultados[i] = score;
            break;
        }
    }

    // Mostrar os resultados finais
    cout << "\nResultados das rodadas:\n";
    for (int i = 0; i < numRodadas; ++i) {
        cout << "Rodada " << i + 1 << ":\n";
        cout << fixed << setprecision(3);  // Configurar para exibir 3 casas decimais
        cout << "  Tempo de execucao: " << temposDeExecucao[i] << " segundos\n";
        if (resultados[i].first == INT_MIN || resultados[i].second == INT_MIN) {
            cout << "  Pontuacao Jogador 1: N/A" << endl;
            cout << "  Pontuacao Jogador 2: N/A" << endl;
        } else {
            cout << "  Pontuacao Jogador 1: " << resultados[i].first << endl;
            cout << "  Pontuacao Jogador 2: " << resultados[i].second << endl;
        }
    }

    return 0;
}