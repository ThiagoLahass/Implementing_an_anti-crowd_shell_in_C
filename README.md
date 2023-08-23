# Implementing_an_anti-crowd_shell_in_C

Implementação na linguagem C um shell denominado acsh (anti-crowd shell) para
colocar em prática os princípios de manipulação de processos.

Ao iniciar, acsh exibe seu prompt (os símbolos no começo de cada linha indicando que o shell está
à espera de comandos). Quando ele recebe uma linha de comando do usuário, é iniciado seu
processamento. Primeiro, a linha deve ser interpretada em termos da linguagem de comandos
definida a seguir e cada comando identificado deve ser executado. Essa operação possivelmente
levará ao disparo de novos processos.

Um primeiro diferencial do acsh é que, na linha de comando, o usuário pode solicitar a criação de
um ou mais processos:
 - acsh> comando1 <3 comando2 <3 comando3
   
Mas ao contrário dos shells UNIX tradicionais, ao processar um comando, ou um conjunto de
comandos, ele deverá criar o(s) processo(s) para executar o(s) respectivo(s) comando(s) em uma
sessão separada da sessão do acsh. Isso é justamente para evitarmos aglomerações de processos…
apenas pequenos grupos dentro de cada sessão! Assim, no exemplo dado acima, o acsh deverá
criar 3 processos – P1 , P2 e P3 – para executar os comandos comando1, comando2 e comando 3
respectivamente (comandoX trata-se de um “comando externo”, como será melhor explicado). E
esses três processos deverão ser “irmãos” e pertencerem a uma mesma sessão, que seja diferente da
sessão do acsh. O número de comandos externos passados em uma mesma linha de comando pode
variar de 1 a 5… lembrando… sem aglomerações! Com isso em uma mesma sessão só deverá haver
processos que tenham sido criados durante uma mesma linha de comando.

Além disso, os processos criados serão executados em background e não estarão associados a
nenhum terminal de controle, consequentemente:

- os processos criados pelo acsh não devem receber nenhum sinal gerado por meio de teclas
do terminal (ex: Ctrl-c);
- o acsh retorna imediatamente para receber novos comandos.
  
Outra particularidade do acsh é que o sinal SIGUSR1 é um sinal muito perigoso e contagioso! Se
um dos processos de background morre devido ao SIGUSR1, os demais processos “irmãos” que se
encontram na mesma sessão (ou seja, que foram criados na mesma linha de comando) devem
morrer de forma coletiva, devido ao mesmo sinal. No exemplo anterior, se, por exemplo, P2
terminar devido a um sinal SIGUSR1, P1 e P3 também devem ser finalizados por meio do mesmo
sinal. Observem que se existirem outros processos de background que tenham sido criados em
outras linhas de comando, eles NÃO deverão morrer, uma vez que eles estavam isolados em sessões
diferentes.

Ah sim! Tem mais… se um processo de background for criado isoladamente, como neste exemplo:

- acsh> comando1
  
... ele não poderá morrer devido ao sinal SIGUSR1… nunca! Afinal, ele está isolado e bem
comportado em uma sessão exclusiva para ele… :-) … isolamento social tem que ter alguma
vantagem!!

Bom… apenas em algumas situações não serão criados processos em background. A primeira delas
é quando o usuário entrar com uma operação interna do shell. Com isso não é criado nenhum novo
processo e o próprio shell executará a operação (detalhes mais à frente). A segunda situação ocorre
quando for utilizado um operador especial ‘%’ no final da linha. Por exemplo:

- acsh> comando1 %

Neste exemplo acima, o acsh deverá criar um processo em foreground e pertencendo à mesma
sessão do acsh. Neste caso, o prompt só será novamente exibido ao final da execução do processo
de foreground criado.
