/**
 * Change this if more than 50 commands are needed
 * There is another limitation in C so this number will never be reached anyway
 */
const uint MAX_COMMANDS = 50;

class RC_Pair {
    String first;
    Racesow_Command @second;

    RC_Pair() {
    }

    RC_Pair(String &in first, Racesow_Command @second) {
        this.first = first;
        @this.second = second;
    }

    void setFirst(String &in first) {
        this.first = first;
    }

    void setSecond(Racesow_Command @second) {
        @this.second = second;
    }

    String @getFirst() {
        return @this.first;
    }

    Racesow_Command @getSecond() {
        return @this.second;
    }

}

/**
 * Map that stores all available commands.
*
* Every gametype has an object of this class and stores all the commands available in it.
*
*/
class RC_Map {
    uint max_size;
    uint last;
    RC_Pair@[] elements;

    RC_Map() {
        this.max_size=MAX_COMMANDS;
        this.last = 0;
        this.elements.resize(max_size); //RC_Pair@[](max_size); <- This doesn't seem to work anymore
    }

    RC_Map(uint size) {
        this.max_size = size;
        this.last = 0;
        this.elements.resize(max_size); //RC_Pair@[](max_size);
    }

    Racesow_Command @get_opIndex(String &in idx) {
        int position;
        if ( (position = this.find(idx)) != -1 ) {
            return @elements[position].getSecond();
        }
        return null;
    }

    void set_opIndex(String &idx, Racesow_Command @idy) {
        if( @idx == null || @idy == null )
            return;
        if( this.last < this.max_size) {
            idy.name = idx; // dunno where else to put this...
            int position;
            if ( ( position = this.find(idx)) != -1 ) {
                this.elements[position].setSecond(idy);
            }
            else
            {
                @this.elements[last++] = @RC_Pair( idx, idy );
            }
        }
        else
            G_Print("================================Warning: Map Full================================\n");
    }

    int find( String &in search) {
        for (uint i = 0; i < this.max_size; i++) {
            if ( @elements[i] != null ) {
                if ( elements[i].getFirst() == search )
                    return i;
            }
            else
            {
                return -1;
            }
        }
        return -1;
    }

    Racesow_Command @getCommandAt( int idx ) {
        return @this.elements[idx].getSecond();
    }

    uint size() {
        return this.last;
    }

    void register() {
        for( uint i = 0; i < this.last; i++) {
            G_RegisterCommand(this.elements[i].getFirst());
        }
    }
}
