import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.Socket;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;

public class RpsClient {
    public static void main(String[] args) {
        // Usage. java RpsClient nomehost porta

        if (args.length != 2) {
            System.err.println("Errore. Uso corretto: java RpsClient nomehost porta");
            System.exit(-1);
        }

        try {
        // Creazione socket stream
        Socket ss = new Socket(args[0], Integer.parseInt(args[1]));
        BufferedReader netIn = new BufferedReader(new InputStreamReader(ss.getInputStream(), StandardCharsets.UTF_8));
        BufferedWriter netOut = new BufferedWriter(new OutputStreamWriter(ss.getOutputStream(), StandardCharsets.UTF_8));
        // Input utente
        BufferedReader kb = new BufferedReader(new InputStreamReader(System.in, StandardCharsets.UTF_8));

        for(;;) {
            // Richiesta all'utente delle opt per ps
            System.out.println("Inserisci gli argomenti per ps");
            String option = kb.readLine();

            // Se l'utente digita . esco
            if (option.equals(".")) {
                break;
            }

            // invio le options al server
            netOut.write(option);
            netOut.newLine();
            netOut.flush();

            // leggo la risposta e la stampo a video
            for(;;) {
                String line;
                // Leggo la stringa
                line = netIn.readLine();
                // Controllo eventuali errori
                if (line == null) {
                    // Se sono qui il server ha chiuso la connessione
                    System.err.println("Il server ha chiuso la connessione!");
                    System.exit(-2);
                }
                // Stampo a video il risultato
                System.out.println(line);

                if (line.equals("FINE")) {
                    // Il server ha smesso di inviare posso passare ad una nuova richiesta
                    break;
                }
            }
        }

        // Chiudo la socket
        ss.close();

        } catch (IOException e) {
            System.err.println(e.getMessage());
            e.printStackTrace();
            System.exit(-2);
        }
    }
}