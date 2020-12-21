import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.Socket;
import java.nio.charset.StandardCharsets;

public class Client {

    public static void main(String[] args) {
        // java Client nomehost porta

        // Controllo argomenti
        if (args.length != 2) {
            System.err.println("Errore. Uso corretto: java Client nomehost porta");
            System.exit(1);
        }

        try {

            // Creazione della socket
            Socket sd = new Socket(args[0], Integer.parseInt(args[1]));
            BufferedReader netIn = new BufferedReader(
                    new InputStreamReader(sd.getInputStream(), StandardCharsets.UTF_8));
            BufferedWriter netOut = new BufferedWriter(
                    new OutputStreamWriter(sd.getOutputStream(), StandardCharsets.UTF_8));

            // BufferedReader per input utente
            BufferedReader kb = new BufferedReader(new InputStreamReader(System.in, StandardCharsets.UTF_8));
            
            for(;;) {
                String nomeVino, annataVino;
                // Richiesta input utente
                System.out.println("Inserire il nome del vino:");
                nomeVino = kb.readLine();
                
                if (nomeVino.equals(".\n")) {
                    break;
                }
                
                System.out.println("Inserire l'annata del vino:");
                annataVino = kb.readLine();

                if (annataVino.equals(".\n")) {
                    break;
                }


                // Invio le informazioni al server
                netOut.write(nomeVino);
                netOut.newLine();
                netOut.flush();

                netOut.write(annataVino);
                netOut.newLine();
                netOut.flush();

                // Ciclo di lettura dal server
                for(;;) {
                    String line;
                    line = netIn.readLine();

                    System.out.println(line);

                    if (line.equals("FINE")) {
                        break;
                    }
                }

            }

            // Chiusura della socket
            sd.close();
            
        } catch (IOException e) {
            System.err.println(e.getMessage());
            e.getLocalizedMessage();
            System.exit(2);
        }
    }
}