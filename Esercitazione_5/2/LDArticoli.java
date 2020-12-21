import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.Socket;
import java.nio.charset.StandardCharsets;

public class LDArticoli {
	public static void main(String[] args) {
		/** java LDArticoli host porta */
		String email, pass, nomeRivista, ack, autorizzazione, line;

		/** Controllo argomenti */
		if (args.length != 2) {
			System.err.println("Errore. Uso corretto: java LDArticoli host porta");
			System.exit(1);
		}

		try {

			/** Creazione della socket */
			Socket sd = new Socket(args[0], Integer.parseInt(args[1]));
			BufferedReader netIn = new BufferedReader(
					new InputStreamReader(sd.getInputStream(), StandardCharsets.UTF_8));
			BufferedWriter netOut = new BufferedWriter(
					new OutputStreamWriter(sd.getOutputStream(), StandardCharsets.UTF_8));
			/** Input utente */
			BufferedReader kb = new BufferedReader(new InputStreamReader(System.in));

			/** Richiesta informazioni all'utente */
			System.out.println("Inserire la email ('fine' per terminare):");
			email = kb.readLine();

			/** Controllo il valore di email */
			if (email.equals("fine")) {
				System.exit(0);
			}

			System.out.println("Inserire la password ('fine' per terminare):");
			pass = kb.readLine();

			/** Controllo il valore di password */
			if (pass.equals("fine")) {
				System.exit(0);
			}

			/** Invio l'email */
			netOut.write(email);
			netOut.newLine();
			netOut.flush();

			/** Ricevo ack */
			ack = netIn.readLine();
			if (!ack.equals("OK")) {
				System.err.println("Unexpected error occured.");
				System.exit(2);
			}

			/** Invio la password */
			netOut.write(pass);
			netOut.newLine();
			netOut.flush();

			/** Verifica autorizzazione */
			autorizzazione = netIn.readLine();
			if (!autorizzazione.equals("1")) {
				System.out.println("Attenzione, email o password errati. Riprovare.");
				System.exit(3);
			}

			for (;;) {
				/** Richiedo all'utente il nome della rivista */
				System.out.println("Inserisci il nome della rivista di interesse ('fine' per terminare):");
				nomeRivista = kb.readLine();

				/** Controllo il valore di email */
				if (nomeRivista.equals("fine")) {
					System.exit(0);
				}

				/** Invio il nome della rivista al server */
				netOut.write(nomeRivista);
				netOut.newLine();
				netOut.flush();

				/** Leggo le informazioni dal server */
				for (;;) {
					line = netIn.readLine();
					/** Verifico che il server non abbia finito di mandare */
					if (line.equals("FINE")) {
						break;
					}
					System.out.println(line);
				}
			}
		} catch (IOException e) {
			System.err.println(e.getMessage());
			e.printStackTrace();
			System.exit(4);
		}
	}
}