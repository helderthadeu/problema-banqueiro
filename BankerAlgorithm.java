import java.util.Random; //Importa a classe Random para gerar números aleatórios
import java.util.concurrent.locks.ReentrantLock; //Importa a classe de bloqueio para controlar o acesso concorrente

//Classe principal que implementa o Algoritmo do Banqueiro
public class BankerAlgorithm{

    //Constantes pedidas (obrigatórias)
    private static final int NUMBER_OF_CUSTOMERS = 5;
    private static final int NUMBER_OF_RESOURCES = 3;
    private final int[] available;
    private final int[][] maximum;
    private final int[][] allocation;
    private final int[][] need;
    private final ReentrantLock lock = new ReentrantLock();

    //Construtor da classe que recebe os recursos totais disponíveis no sistema
    public BankerAlgorithm(int[] resources){
        available = resources.clone(); //Clona o vetor de recursos disponíveis
        maximum = new int[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
        allocation = new int[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
        need = new int[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

        Random rand = new Random();

        //Inicializa as matrizes de máximo e necessidade com valores aleatórios
        for(int i = 0; i < NUMBER_OF_CUSTOMERS; i++){
            for(int j = 0; j < NUMBER_OF_RESOURCES; j++){
                maximum[i][j] = rand.nextInt(resources[j] + 1); //Gera um valor aleatório no intervalo [0, recursos[j]]
                need[i][j] = maximum[i][j]; //Inicialmente, o cliente ainda não recebeu recursos
            }
        }
    }

    //Método que trata o pedido de recursos de um cliente
    public boolean request_resources(int customer_num, int[] request){
        lock.lock(); //Garante acesso exclusivo ao método
        try{
            System.out.println("Cliente " + customer_num + " solicitou recursos: " + arrayToString(request));

            //Verifica se o pedido é válido (não excede o necessário nem o disponível)
            for(int i = 0; i < NUMBER_OF_RESOURCES; i++){
                if(request[i] > need[customer_num][i] || request[i] > available[i]){
                    System.out.println("Solicitação negada para Cliente " + customer_num);
                    return false;
                }
            }

            //Aloca temporariamente os recursos
            for(int i = 0; i < NUMBER_OF_RESOURCES; i++){
                available[i] -= request[i];
                allocation[customer_num][i] += request[i];
                need[customer_num][i] -= request[i];
            }

            //Verifica se o sistema continua em estado seguro
            if(!isSafeState()){
                //Se não estiver seguro, desfaz a alocação
                for(int i = 0; i < NUMBER_OF_RESOURCES; i++){
                    available[i] += request[i];
                    allocation[customer_num][i] -= request[i];
                    need[customer_num][i] += request[i];
                }
                System.out.println("Sistema ficou inseguro! Revertendo solicitação do Cliente " + customer_num);
                return false;
            }

            //Caso tudo esteja seguro, confirma a alocação
            System.out.println("Solicitação aprovada para Cliente " + customer_num);
            return true;
        }finally{
            lock.unlock(); //Libera o bloqueio
        }
    }

    //Método para liberar recursos após uso
    public void release_resources(int customer_num, int[] release){
        lock.lock(); //Garante acesso exclusivo
        try{
            System.out.println("Cliente " + customer_num + " liberou recursos: " + arrayToString(release));

            //Atualiza as estruturas de dados
            for(int i = 0; i < NUMBER_OF_RESOURCES; i++){
                allocation[customer_num][i] -= release[i];
                available[i] += release[i];
                need[customer_num][i] += release[i];
            }
        }finally{
            lock.unlock(); //Libera o bloqueio
        }
    }

    //Verifica se o sistema está em um estado seguro (algoritmo de segurança do banqueiro)
    private boolean isSafeState(){
        int[] work = available.clone(); //Vetor de trabalho com os recursos disponíveis
        boolean[] finish = new boolean[NUMBER_OF_CUSTOMERS]; //Marca se um cliente pode terminar

        //Tenta encontrar uma sequência segura
        for(int count = 0; count < NUMBER_OF_CUSTOMERS; count++){
            for(int i = 0; i < NUMBER_OF_CUSTOMERS; i++){
                if(!finish[i]){
                    boolean canProceed = true;

                    //Verifica se o cliente i pode ser atendido com os recursos disponíveis
                    for(int j = 0; j < NUMBER_OF_RESOURCES; j++){
                        if(need[i][j] > work[j]){
                            canProceed = false;
                            break;
                        }
                    }

                    //Se pode, simula o término e libera seus recursos
                    if(canProceed){
                        for(int j = 0; j < NUMBER_OF_RESOURCES; j++){
                            work[j] += allocation[i][j];
                        }
                        finish[i] = true;
                    }
                }
            }
        }

        //Se todos puderam terminar, o sistema está seguro
        for(boolean f : finish){
            if(!f) return false;
        }
        return true;
    }

    //Main
    public static void main(String[] args){
        int[] resources ={10, 5, 7}; //Quantidade total de cada recurso
        BankerAlgorithm banker = new BankerAlgorithm(resources); //Cria o objeto do banqueiro

        Thread[] customers = new Thread[NUMBER_OF_CUSTOMERS];
        Random rand = new Random();

        //Cria uma thread para cada cliente
        for(int i = 0; i < NUMBER_OF_CUSTOMERS; i++){
            final int customerNum = i;
            
            customers[i] = new Thread(() ->{
                try{
                    int count = 0;
                    while (count != 20){
                        int[] request = new int[NUMBER_OF_RESOURCES];

                        //Gera uma solicitação aleatória 
                        for(int j = 0; j < NUMBER_OF_RESOURCES; j++){
                            request[j] = rand.nextInt(banker.need[customerNum][j] + 1);
                        }

                        //Tenta solicitar recursos
                        if(banker.request_resources(customerNum, request)){
                            Thread.sleep(rand.nextInt(1000)); //Simula tempo de uso
                            banker.release_resources(customerNum, request); //Libera os recursos
                        }
                        count++;
                    }
                }catch (InterruptedException e){
                    Thread.currentThread().interrupt(); //Em caso de interrupção
                }
            });
            customers[i].start(); //Inicia a thread
        }
    }

    //Utils para converter um array em String
    private static String arrayToString(int[] array){
        StringBuilder sb = new StringBuilder("[");
        for(int i = 0; i < array.length; i++){
            sb.append(array[i]);
            if(i < array.length - 1) sb.append(", ");
        }
        sb.append("]");
        return sb.toString();
    }
}