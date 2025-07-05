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
#include <unistd.h> 
#include <map>
#include <fstream>

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

    int chooseCardToPlayForPlayer1() {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> distr(0, hand.size() - 1);
        int index = distr(gen);
        //cout << "valor do aleatorio: " << index << endl;
        return index;
    }

    int chooseCardToPlayForPlayer2() {
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

    int chooseCardToPlayForPlayer3(const vector<stack<Card>>& expeditions) {
        int bestCardIndex = -1;
        int highestPotentialPoints = INT_MIN;
        int lowestCardValue = INT_MAX;
        int lowestCardIndex = -1;

        int bestProxCardIndex = -1;
        int closestValueDifference = INT_MAX;

        // Itera sobre todas as cartas na mão
        for (int i = 0; i < hand.size(); i++) {
            const Card& card = hand[i];

            // Atualiza o índice da carta de menor valor
            if (card.value < lowestCardValue) {
                lowestCardValue = card.value;
                lowestCardIndex = i;
            }

            // Verifica se a carta pode ser jogada em expedições existentes
            for (int j = 0; j < expeditions.size(); j++) {
                if (!expeditions[j].empty() && expeditions[j].top().color == card.color) {
                    const Card& topExpeditionCard = expeditions[j].top();

                    // Calcula a diferença entre o valor da carta da mão e o valor da carta no topo da expedição
                    int valueDifference = abs(card.value - topExpeditionCard.value);

                    // Verifica se a carta da mão é a mais próxima do valor no topo da expedição
                    if (valueDifference < closestValueDifference || 
                        (valueDifference == closestValueDifference && card.value < hand[bestProxCardIndex].value)) {
                        closestValueDifference = valueDifference;
                        bestProxCardIndex = i;
                    }
                }
            }
        }

        // Se encontrar a melhor carta próxima para jogar, retorna o índice dela
        if (bestProxCardIndex != -1) {
            return bestProxCardIndex;
        }

        // Se encontrar a melhor carta para jogar, retorna o índice dela
        if (bestCardIndex != -1) {
            return bestCardIndex;
        }

        // Se nenhuma jogada for encontrada, retorna o índice da carta de menor valor
        return lowestCardIndex;
    }

    // Função para decisão do primeiro jogador (escolha aleatória)
    bool chooseRandomly() {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(0, 1);
        return dis(gen);  // Retorna true para puxar do descarte, false para puxar do deck
    }

    // Função para o segundo jogador (verifica uma pilha e escolhe com base na media do valor da mao)
    bool chooseBasedOnDiscardAndMedia(const queue<Card>& discardPile, const vector<Card>& hand) {
        Card cardTopo = discardPile.front();
        int expedition = cardTopo.color;

        // Calcular a soma das cartas da mão na mesma expedição
        double sum = 0.0;
        int count = 0;
        
        for (Card card : hand) {
            if (card.color == expedition) {
                sum += card.value;
                count++;
            }
        }

        // Se não há cartas da mesma expedição na mão, é seguro puxar do descarte
        if (count == 0) {
            return true;
        }

        // Calcular a média das cartas da mesma expedição na mão
        double average = sum / count;

        // Retornar true se a carta do topo do descarte é maior que a média
        return cardTopo.value > average;
    }

    // Função para o terceiro jogador (verifica todas as pilhas)
    bool chooseBasedOnDiscardAndExpedition(const vector<queue<Card>>& discardPiles, const vector<Card>& hand) {
        map<int, pair<int, int>> expeditionStats;

        // Calcula a média das cartas em mãos por expedição
        for (const Card& card : hand) {
            int color = card.color;
            expeditionStats[color].first += card.value;
            expeditionStats[color].second++;
        }

        // Verifica se é melhor puxar do descarte
        for (size_t i = 0; i < discardPiles.size(); ++i) {
            if (discardPiles[i].empty()) continue;

            const Card& topDiscard = discardPiles[i].front();  // Acesso ao topo da fila
            int discardExpedition = topDiscard.color;

            if (expeditionStats.find(discardExpedition) == expeditionStats.end()) continue;

            const auto& stats = expeditionStats[discardExpedition];
            int totalValue = stats.first;
            int count = stats.second;

            double averageValue = (count > 0) ? static_cast<double>(totalValue) / count : 0.0;

            if (topDiscard.value > averageValue) {
                return true;  
            }
        }

        return false;  
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
        if (card.value == -1) {  // Se for uma carta especial (multiplicador)
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
    }

    void dealInitialCards() {
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 8; ++j) {
                players[i].drawCard(deck.back());
                deck.pop_back();
            }
        }
    }

    pair<int, int> choosePlayersForMatch() {
        int player1Type, player2Type;

        player1Type = 1;
        cout << "jogador escolhido: " << player1Type << endl;
        player2Type = 2;
        cout << "jogador escolhido: " << player2Type << endl;

        return make_pair(player1Type, player2Type);
    }

    //função de decisao de puxar dos jogadores
    void drawCardDecision(int currentPlayerType, Player& player, vector<queue<Card>>& discardPiles, vector<Card>& deck) {
        if (!deck.empty()) {
            int discardChoice = -1;
            for (int i = 0; i < 5; ++i) {
                if (!discardPiles[i].empty()) {
                    discardChoice = i;
                    break;
                }
            }

            if (discardChoice != -1) {
                if (currentPlayerType == 0) {
                    if (player.chooseRandomly()) {
                        //cout << "puxou do descarte AI00." << endl;
                        Card cardToDraw = discardPiles[discardChoice].front();
                        discardPiles[discardChoice].pop();
                        player.drawCard(cardToDraw);
                    } else {
                        Card cardToDraw = deck.back();
                        deck.pop_back();
                        player.drawCard(cardToDraw);
                    }
                } else if (currentPlayerType == 1) {
                    if (player.chooseBasedOnDiscardAndMedia(discardPiles[discardChoice], player.hand)) {
                        //cout << "puxou do descarte AI01." << endl;
                        Card cardToDraw = discardPiles[discardChoice].front();
                        discardPiles[discardChoice].pop();
                        player.drawCard(cardToDraw);
                    } else {
                        Card cardToDraw = deck.back();
                        deck.pop_back();
                        player.drawCard(cardToDraw);
                    }
                } else if (currentPlayerType == 2) {
                    if (player.chooseBasedOnDiscardAndExpedition(discardPiles, player.hand)) {
                        //cout << "puxou do descarte AI02." << endl;
                        Card cardToDraw = discardPiles[discardChoice].front();
                        discardPiles[discardChoice].pop();
                        player.drawCard(cardToDraw);
                    } else {
                        Card cardToDraw = deck.back();
                        deck.pop_back();
                        player.drawCard(cardToDraw);
                    }
                }
            } else {
                Card cardToDraw = deck.back();
                deck.pop_back();
                player.drawCard(cardToDraw);
            }
        }
    }

    //com 10 cartas ou menos priorizar fazer maior numero de pontos
    void takeTurn(pair<int, int> playerTypes) {
        int cardIndex;
        int currentPlayerType = (currentPlayer == 0) ? playerTypes.first : playerTypes.second;

        if (currentPlayerType == 0) {
            cardIndex = players[currentPlayer].chooseCardToPlayForPlayer1();
        } else if (currentPlayerType == 1) {
            cardIndex = players[currentPlayer].chooseCardToPlayForPlayer2();
        } else {
            cardIndex = players[currentPlayer].chooseCardToPlayForPlayer3(players[currentPlayer].expeditions);
        }

        int expeditionIndex = -1;

        if (deck.size() > 10) {
            for (int i = 0; i < 5; ++i) {
                if (players[currentPlayer].canPlayCard(cardIndex, i)) {
                    expeditionIndex = i;
                    break;
                }
            }
        } else {
            int maxPoints = -1000;
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

        if (expeditionIndex >= 0 && expeditionIndex < 5) {
            players[currentPlayer].playCard(cardIndex, expeditionIndex);
        } else {
            //cout << "Descartou carta." << endl; 
            players[currentPlayer].discardCard(cardIndex, discardPiles);
        }

        drawCardDecision(currentPlayerType, players[currentPlayer], discardPiles, deck);

        currentPlayer = (currentPlayer + 1) % 2;
    }

    bool isGameOver() {
        return deck.empty();
    }

    void playGame() {
        pair<int, int> playerTypes = choosePlayersForMatch(); // escolhe os jogares da partida
        while (!isGameOver()) {
            takeTurn(playerTypes);
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

    int numRodadas = 20;
    //cout << "Digite o numero de rodadas: ";
    //cin >> numRodadas;

    vector<double> temposDeExecucao(numRodadas);
    vector<pair<int, int>> resultados(numRodadas);  // Armazena pontuação dos jogadores

    auto inicio = high_resolution_clock::now();
    

    for (int i = 0; i < numRodadas; ++i){
        sleep(1); // Espera 1 segundo para alterar o rand do embaralhar deck

        auto start = high_resolution_clock::now();
        pair<int, int> score = {INT_MIN, INT_MIN};  // Inicializa pontuações como nulas
        cout << "Teste: " << i + 1 << endl;

        while (true) {
            auto now = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(now - start).count();

            if (duration > 10000) {  // 10000 ms = 10 segundos
                cout << "Rodada " << i + 1 << " excedeu o limite de 10 segundos. Jogo encerrado." << endl;
                temposDeExecucao[i] = 10.000;
                resultados[i] = {0, 0}; // Define pontuação como 0
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
    auto fim = high_resolution_clock::now();
    double tempoTotal = duration_cast<milliseconds>(fim - inicio).count();;
    int pontTotal01 = 0;
    int pontTotal02 = 0;

    //Gravar os resultados finais em um arquivo
    ofstream outputFile("resultados.txt");
    if (outputFile.is_open()) {
        outputFile << fixed << setprecision(10);  // Configurar para exibir 5 casas decimais
        outputFile << "\nResultados das rodadas:\n";
        for (int i = 0; i < numRodadas; i++){
            //tempoTotal += temposDeExecucao[i];
            pontTotal01 += resultados[i].first;
            pontTotal02 += resultados[i].second;
        }
        outputFile << "Tempo total de execução: " << tempoTotal/1000.0 << " ms\n";
        outputFile << "Pontuação total Jogador 1: " << pontTotal01 << endl;
        outputFile << "Pontuação total Jogador 2: " << pontTotal02 << endl;
        // for (int i = 0; i < numRodadas; ++i) {
        //     outputFile << "Rodada " << i + 1 << ":\n";
        //     outputFile << "  Tempo de execucao: " << temposDeExecucao[i] << " segundos\n";
        //     if (resultados[i].first == INT_MIN || resultados[i].second == INT_MIN) {
        //         outputFile << "  Pontuacao Jogador 1: N/A" << endl;
        //         outputFile << "  Pontuacao Jogador 2: N/A" << endl;
        //     } else {
        //         outputFile << "  Pontuacao Jogador 1: " << resultados[i].first << endl;
        //         outputFile << "  Pontuacao Jogador 2: " << resultados[i].second << endl;
        //     }
        // }
        outputFile.close();  // Fecha o arquivo
    } else {
        cout << "Não foi possível abrir o arquivo para escrita." << endl;
    }
    // Mostrar os resultados finais
    /*cout << "\nResultados das rodadas:\n";
    for (int i = 0; i < numRodadas; ++i) {
        cout << "Rodada " << i + 1 << ":\n";
        cout << fixed << setprecision(5);  // Configurar para exibir 5 casas decimais
        cout << "  Tempo de execucao: " << temposDeExecucao[i] << " segundos\n";
        if (resultados[i].first == INT_MIN || resultados[i].second == INT_MIN) {
            cout << "  Pontuacao Jogador 1: N/A" << endl;
            cout << "  Pontuacao Jogador 2: N/A" << endl;
        } else {
            cout << "  Pontuacao Jogador 1: " << resultados[i].first << endl;
            cout << "  Pontuacao Jogador 2: " << resultados[i].second << endl;
        }
    }*/
}