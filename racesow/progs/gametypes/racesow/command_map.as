/**
 * Change this if more than 50 commands are needed
 * There is another limitation in C so this number will never be reached anyway
 */
const uint MAX_COMMANDS = 50;

class RC_Pair {
    cString first;
    Racesow_Command @second;

    RC_Pair(cString &in first, Racesow_Command @second) {
        this.first = first;
        @this.second = second;
    }

    void setFirst(cString &in first) {
        this.first = first;
    }

    void setSecond(Racesow_Command @second) {
        @this.second = second;
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
* FIXME: This and the parts that use this will be extended when i get the new angelwrap working ;)
*
*/

class RC_Map {
    uint max_size;
    uint last;
    RC_Pair@[] elements;

    RC_Map() {
        this.max_size=MAX_COMMANDS;
        this.last = 0;
        this.elements.resize(max_size);
    }

    ~RC_Map() {
        for( uint i = 0; i < this.last; i++)
            @this.elements[i] = null;
    }

    Racesow_Command @get_opIndex(cString &in idx) {
        int position;
        if ( (position = this.find(idx)) != -1 ) {
            return @elements[position].getSecond();
        }
        return null;
    }

    void set_opIndex(cString &idx, Racesow_Command @idy) {
        if ( @idx != null && @idy != null && this.last != this.max_size) {
            int position;
            if ( ( position = this.find(idx)) != -1 ) {
                this.elements[position].setSecond(idy);
            }
            else
            {
                @this.elements[last++] = @RC_Pair( idx, idy );
            }
        }
    }

    int find( cString &in search) {
        for (uint i = 0; i < this.max_size; i++) {
            if ( @elements[i] != null ) {
                if ( elements[i].first == search )
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
            G_RegisterCommand(this.elements[i].first);
        }
    }
}
