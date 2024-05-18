import com.jpexs.decompiler.flash.SWF;
import com.jpexs.decompiler.flash.tags.FileAttributesTag;
import com.jpexs.decompiler.flash.tags.FrameLabelTag;
import com.jpexs.decompiler.flash.tags.Tag;
import com.jpexs.decompiler.flash.tags.base.PlaceObjectTypeTag;
import com.jpexs.decompiler.flash.types.MATRIX;

import java.io.*;
import java.util.LinkedList;
import java.util.Queue;
import java.util.concurrent.LinkedBlockingQueue;

public class Main {
     public static void add_frame_labels(String path, String path_out, Queue<String> labels) {
        try (FileInputStream fis = new FileInputStream(path)) { //open up a file
            SWF swf = new SWF(fis, true);
            LinkedList<PlaceObjectTypeTag> tag_list = new LinkedList<>();

            for (Tag t : swf.getTags()) {
                if (t instanceof PlaceObjectTypeTag po) { //Find all PlaceObject(1,2,3,4) tags
                    MATRIX mat = po.getMatrix();
                    if (mat != null) {
                        mat.hasScale = true;
                        mat.scaleX = 262144; //read these from ffdec after applying a 400% scale 32->128
                        mat.scaleY = 262144;
                        mat.translateX = 0;
                        mat.translateY = 0;
                    } else {
                        System.out.println("Mat is null");
                    }
                    po.setModified(true); //CRUCIAL to call on every modified tag, otherwise it won't save
                    tag_list.add(po);
                }
            }

            for (PlaceObjectTypeTag po: tag_list) {
                int index = swf.indexOfTag(po);
                FrameLabelTag tag = new FrameLabelTag(swf);
                tag.name = labels.remove();
                tag.setModified(true);
                swf.addTag(index, tag);
            }
            swf.frameRate = 24.0F;
            /*swf.version=10;
            FileAttributesTag fa = new FileAttributesTag(swf);
            fa.reservedA=false;
            fa.useDirectBlit=false;
            fa.useGPU=false;
            fa.hasMetadata=false;
            fa.actionScript3=false;
            fa.useNetwork=false;
            fa.noCrossDomainCache=false;
            fa.swfRelativeUrls=false;
            fa.reservedB=0;
            fa.forceWriteAsLong=false;
            swf.addTag(fa);*/
            swf.setModified(true);

            OutputStream os = new FileOutputStream(path_out);
            try {
                swf.saveTo(os);
            } catch (IOException e) {
                System.out.println("ERROR: Error during SWF saving");
            }

        } catch (IOException | InterruptedException e) {
            throw new RuntimeException(e);
        }
    }

    public static void _load_hardcoded()
    {
        String swf_path = "F:\\Skyrim Dev\\WORK\\TMP\\tmp_spell_icons.swf";
        String swf_path_out = "F:\\Skyrim Dev\\ADT\\mods\\Spell Hotbar\\Interface\\SpellHotbar\\spell_icons.swf";
        String label_list = "F:\\Skyrim Dev\\WORK\\TMP\\frame_labels.txt";

        Queue<String> labels = new LinkedBlockingQueue<>();
        BufferedReader reader;
        try {
            reader = new BufferedReader(new FileReader(label_list));
            String line = reader.readLine();
            while (line != null) {
                labels.add(line);
                // read next line
                line = reader.readLine();
            }
            reader.close();

            add_frame_labels(swf_path, swf_path_out, labels);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public static void main(String[] args) {
        if (args.length < 3) {
            System.out.println("Not Enough arguments, usage: <Path in> <Path Out> <labels>...");
            return;
        }
        String swf_path = args[0];
        String swf_path_out = args[1];
        Queue<String> labels = new LinkedBlockingQueue<>();

        for (int i=2; i< args.length; i++)
        {
            labels.add(args[i]);
        }
        add_frame_labels(swf_path, swf_path_out, labels);
    }
}