import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

public class Prova {
    public static void main(String[] args) {
        
        String line = null;
        
        BufferedReader kb = new BufferedReader(new InputStreamReader(System.in));
        try {
            line = kb.readLine();
            line += "\n";
        } catch(IOException e) {
            e.printStackTrace();
            System.exit(1);
        }

        System.out.println(line.length());
        for(int i = 0; i < line.length(); ++i) {
            System.out.print(line.charAt(i));
        }

        int i = 1, length = 0;
        while(line.charAt(i-1) != '\n') {
            ++length;
            ++i;
        }
        System.out.println(i);


    }
}
