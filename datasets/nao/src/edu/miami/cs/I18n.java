package edu.miami.cs;

import java.text.MessageFormat;

/**
 * Author: Sam Abeyruwan
 */
public final class I18n {

    public static String _(String s) {
        return s; // FixMe: This could be something from i18n; we are not worried about this now
    }

    public static String format(String fmt, Object... args) {
        return MessageFormat.format(fmt, args);
    }
}
