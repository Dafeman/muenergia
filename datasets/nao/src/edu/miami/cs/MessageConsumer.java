package edu.miami.cs;

/**
 * Author: Sam Abeyruwan
 */

/**
 * Interface for dealing with parser/compiler output.
 * <p/>
 * Different instances of MessageStream need to do different things with
 * messages.  In particular, a stream instance used for parsing output from
 * the compiler compiler has to interpret its messages differently than one
 * parsing output from the runtime.
 * <p/>
 * Classes which consume messages and do something with them
 * should implement this interface.
 */
public interface MessageConsumer {

    public void message(String s);

}