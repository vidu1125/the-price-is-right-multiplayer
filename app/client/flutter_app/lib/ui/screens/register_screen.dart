import 'package:flutter/material.dart';
import '../theme/login_theme.dart';
import '../../services/service_locator.dart';

class RegisterScreen extends StatefulWidget {
  const RegisterScreen({super.key});

  @override
  State<RegisterScreen> createState() => _RegisterScreenState();
}

class _RegisterScreenState extends State<RegisterScreen> {
  final _formKey = GlobalKey<FormState>();
  final _emailController = TextEditingController();
  final _nameController = TextEditingController();
  final _passwordController = TextEditingController();
  bool _isLoading = false;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Stack(
        children: [
          // Background Image
          Positioned.fill(
            child: Image.asset("assets/images/lobby-bg.png", fit: BoxFit.cover),
          ),
          Positioned.fill(child: Container(color: Colors.black.withOpacity(0.5))),

          Center(
            child: SingleChildScrollView(
              padding: const EdgeInsets.all(24),
              child: Container(
                constraints: const BoxConstraints(maxWidth: 480),
                padding: const EdgeInsets.all(40),
                decoration: LoginTheme.cardDecoration,
                child: Form(
                  key: _formKey,
                  child: Column(
                    mainAxisSize: MainAxisSize.min,
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      const Text("Sign Up", style: LoginTheme.headingStyle),
                      const SizedBox(height: 8),
                      const Text("Create a new account and join the fun!", 
                        style: TextStyle(color: LoginTheme.textSecondary)),
                      const SizedBox(height: 32),
                      
                      // Email Field
                      const Text("Email", style: LoginTheme.labelStyle),
                      const SizedBox(height: 8),
                      TextFormField(
                        controller: _emailController,
                        decoration: LoginTheme.inputDecoration("you@example.com"),
                        validator: (val) => (val == null || !val.contains('@')) ? "Invalid email" : null,
                      ),
                      const SizedBox(height: 24),

                      // Name Field
                      const Text("Display Name", style: LoginTheme.labelStyle),
                      const SizedBox(height: 8),
                      TextFormField(
                        controller: _nameController,
                        decoration: LoginTheme.inputDecoration("Your in-game name"),
                        validator: (val) => (val == null || val.isEmpty) ? "Name is required" : null,
                      ),
                      const SizedBox(height: 24),
                      
                      // Password Field
                      const Text("Password", style: LoginTheme.labelStyle),
                      const SizedBox(height: 8),
                      TextFormField(
                        controller: _passwordController,
                        obscureText: true,
                        decoration: LoginTheme.inputDecoration("Min 6 characters"),
                        validator: (val) => (val == null || val.length < 6) ? "Min 6 characters" : null,
                      ),
                      
                      const SizedBox(height: 32),
                      
                      // CTA Button
                      GestureDetector(
                        onTap: _isLoading ? null : () async {
                          if (_formKey.currentState!.validate()) {
                            setState(() => _isLoading = true);
                            
                            final result = await ServiceLocator.authService.register(
                              email: _emailController.text,
                              name: _nameController.text,
                              password: _passwordController.text,
                            );

                            setState(() => _isLoading = false);

                            if (result["success"] == true) {
                              if (mounted) {
                                // After success, we might want to auto-login or go back to login
                                ScaffoldMessenger.of(context).showSnackBar(
                                  const SnackBar(content: Text("Account created! Please sign in.")),
                                );
                                Navigator.pop(context);
                              }
                            } else {
                              if (mounted) {
                                ScaffoldMessenger.of(context).showSnackBar(
                                  SnackBar(
                                    content: Text(result["error"] ?? "Registration failed"),
                                    backgroundColor: Colors.red,
                                  ),
                                );
                              }
                            }
                          }
                        },
                        child: Container(
                          width: double.infinity,
                          height: 55,
                          decoration: BoxDecoration(
                            borderRadius: BorderRadius.circular(14),
                            gradient: LoginTheme.ctaGradient,
                          ),
                          alignment: Alignment.center,
                          child: _isLoading 
                            ? const CircularProgressIndicator(color: Colors.white)
                            : const Text("Create Account", 
                                style: TextStyle(color: Colors.white, fontSize: 18, fontWeight: FontWeight.bold)),
                        ),
                      ),
                      
                      const SizedBox(height: 24),
                      
                      // Footer
                      Row(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          const Text("Already have an account? ", style: TextStyle(color: LoginTheme.textSecondary)),
                          TextButton(
                            onPressed: () => Navigator.pop(context),
                            child: const Text("Sign in", 
                              style: TextStyle(color: LoginTheme.darkBlue, fontWeight: FontWeight.bold)),
                          ),
                        ],
                      ),
                    ],
                  ),
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }
}
