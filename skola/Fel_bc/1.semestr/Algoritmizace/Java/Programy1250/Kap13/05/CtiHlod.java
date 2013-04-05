/* Soubor Kap13\06\CtiHlod.java
 * P�e�te �esk� text v k�dov�n� Latin 2 ze souboru a vyp�e na konzolu.
 * P�edpokl�d� existenci souboru data.dta v aktu�ln�m adres��i, kter� obsahuje
 * �esk� text v k�dov�n� Latin 2 (CP 852).
 * Jako vstup lze pou��t nap�. soubor vytvo�en� programem Hlod.java, 
 * kter� najdete v tomto adres��i.
 */

import java.io.*;

public class CtiHlod {

  public static void main(String[] args) throws Exception{
    File f = new File("data.dta");
    if(!f.exists()) throw new IOException("soubor neexistuje");

    FileInputStream fis = new FileInputStream(f);
    InputStreamReader isr = new InputStreamReader(fis, "Cp852");
    BufferedReader br = new BufferedReader(isr);

    OutputStreamWriter osw = new OutputStreamWriter(System.out, "Cp852");
    PrintWriter pw = new PrintWriter(osw);

    String text;
    while((text = br.readLine()) != null)
    {
    pw.println(text);
    }
    pw.close();
  }
}