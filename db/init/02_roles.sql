-- Roles and Permissions for PostgREST

-- Create a role for anonymous web access (if not exists)
DO $$
BEGIN
  IF NOT EXISTS (SELECT FROM pg_catalog.pg_roles WHERE rolname = 'web_anon') THEN
    CREATE ROLE web_anon NOLOGIN;
  END IF;
END
$$;

GRANT USAGE ON SCHEMA public TO web_anon;
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO web_anon;
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO web_anon;

-- Allow executing functions
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA public TO web_anon;

-- Create a role for authenticated access (if we used it)
-- For now giving web_anon everything is enough for dev.
